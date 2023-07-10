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
#include "common/propagate/vcs_event.h"

struct captured_frame_s;

typedef std::function<void()> subsystem_releaser_t;

extern vcs_event_c<void> k_ev_eco_mode_enabled;
extern vcs_event_c<void> k_ev_eco_mode_disabled;
extern vcs_event_c<const captured_frame_s&> k_ev_frame_processing_finished;

void k_set_eco_mode_enabled(const bool isEnabled);

bool k_is_eco_mode_enabled(void);

#endif
