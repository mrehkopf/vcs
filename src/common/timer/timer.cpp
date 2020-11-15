/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * A subsystem providing basic timer functionality for automatic execution of code
 * at given intervals.
 *
 * Usage:
 *
 *   1. Call kt_initialize_timers() to initialize the timer subsystem.
 *
 *   2. Call kt_timer() to add a new timer. The function takes in an interval and
 *      a function; the function will be executed at about the given intervals.
 *
 *   3. To stop and release all timers, call kt_release_timers(). Individual
 *      timers cannot currently be stopped.
 *
 */

#include <thread>
#include <chrono>
#include <future>
#include <vector>
#include <atomic>
#include <functional>
#include "common/timer/timer.h"

// A timer object, to be used by VCS's timer subsystem (kt_xxxx).
//
// Usage:
//
//   1. Create a new timer object:
//
//      timer_c timer = new timer_c(...);
//
//   2. When you want the timer to stop:
//
//      timer->keepRunning = false; // Ask the timer's thread to exit.
//      timer->future().wait();     // Blocks until the timer's thread exits.
//      delete timer;
//
class timer_c
{
public:
    timer_c(const unsigned intervalMs, std::function<void(void)> func) :
        keepRunning(true),
        intervalMs(intervalMs),
        timeoutFunction(func)
    {
        this->start();

        return;
    }

    ~timer_c()
    {
        return;
    }

    const std::future<void>& future(void) const
    {
        return this->future_;
    }

    std::atomic<bool> keepRunning;

    const unsigned intervalMs;

    const std::function<void(void)> timeoutFunction;

private:
    std::future<void> future_;

    void start(void)
    {
        this->future_ = std::async(std::launch::async, [=]
        {
            while (this->keepRunning)
            {
                unsigned timeWaited = 0;
                
                while (this->keepRunning &&
                       (timeWaited < this->intervalMs))
                {
                    /// TODO: Inform the user that timers' resolution is basically this value.
                    const unsigned waitGranularity = 100;

                    std::this_thread::sleep_for(std::chrono::milliseconds(waitGranularity));
                    timeWaited += waitGranularity;
                }

                this->timeoutFunction();
            }
        });

        return;
    }
};

static std::vector<timer_c*> ACTIVE_TIMERS;

void kt_initialize_timers(void)
{
    return;
}

void kt_release_timers(void)
{
    for (timer_c *const timer: ACTIVE_TIMERS)
    {
        timer->keepRunning = false;
    }

    for (timer_c *const timer: ACTIVE_TIMERS)
    {
        if (timer->future().valid())
        {
            timer->future().wait();
        }

        delete timer;
    }

    return;
}

void kt_timer(const unsigned intervalMs, std::function<void(void)> functionToRun)
{
    ACTIVE_TIMERS.push_back(new timer_c(intervalMs, functionToRun));

    return;
}
