/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_TIMER_TIMER_H
#define VCS_COMMON_TIMER_TIMER_H

#include <functional>

// Runs the given function at the given interval (+ the function's time of
// execution). The timer can't be stopped.
void kt_timer(const unsigned intervalMs, std::function<void(void)> func);

void kt_initialize_timers(void);

void kt_release_timers(void);

#endif
