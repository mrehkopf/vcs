/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_TIMER_TIMER_H
#define VCS_COMMON_TIMER_TIMER_H

#include <chrono>
#include <atomic>
#include <functional>

// An in-thread timer object, to be used by VCS's timer subsystem (kt_xxxx).
//
// Usage:
//
//   1. Create a timer object:
//
//      timer_c timer(...);
//
//   2. Periodically update the timer to keep it running (this will also fire
//      the timer's timeout function when appropriate):
//
//      timer.update();
//
//   3. When you want the timer to stop, just stop calling the .update()
//      method.
//
class timer_c
{
public:
    timer_c(const unsigned intervalMs, std::function<void(const unsigned elapsedMs)> func) :
        intervalMs(intervalMs),
        timeoutFunction(func)
    {
        this->timeOfLastTimeout = std::chrono::system_clock::now();

        return;
    }

    void update(void)
    {
        const auto timeNow = std::chrono::system_clock::now();
        const unsigned msSinceLastTimeout = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - this->timeOfLastTimeout).count();

        if (unsigned(msSinceLastTimeout) >= this->intervalMs)
        {
            this->timeoutFunction(msSinceLastTimeout);
            this->timeOfLastTimeout = timeNow;
        }

        return;
    }

    const unsigned intervalMs;

    const std::function<void(const unsigned elapsedMs)> timeoutFunction;

private:
    std::chrono::system_clock::time_point timeOfLastTimeout = {};
};

// Runs the given function at the given interval (+ the function's time of
// execution). The timer can't be stopped.
void kt_timer(const unsigned intervalMs, std::function<void(const unsigned elapsedMs)> func);

void kt_initialize_timers(void);

void kt_release_timers(void);

void kt_update_timers(void);

#endif
