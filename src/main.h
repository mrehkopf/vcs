/*
 * 2022 Tarpeeksi Hyvae Soft
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

#endif
