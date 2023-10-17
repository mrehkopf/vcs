/*
 * 2022-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_MAIN_H
#define VCS_MAIN_H

#include <vector>
#include <functional>
#include "common/vcs_event/vcs_event.h"

struct captured_frame_s;

typedef std::function<void()> subsystem_releaser_t;

void k_set_eco_mode_enabled(const bool isEnabled);

bool k_is_eco_mode_enabled(void);

// Code running within the main VCS loop is under a lock of the capture mutex.
// In some cases, the code may want to run something after the mutex has been
// released, in which case this function allows to register a callback that
// gets executed in the tail of the main loop after the mutex has been released.
// The callback function is executed once and then removed; if multiple
// callbacks are registered, they're executed in the order of registration.
void k_defer_until_capture_mutex_unlocked(std::function<void(void)> callback);

#endif
