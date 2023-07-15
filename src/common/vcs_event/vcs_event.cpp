/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/vcs_event/vcs_event.h"
#include "capture/capture.h"

vcs_event_c<const captured_frame_s&> kc_ev_new_captured_frame;
vcs_event_c<const video_mode_s&> kc_ev_new_proposed_video_mode;
vcs_event_c<const video_mode_s&> kc_ev_new_video_mode;
vcs_event_c<unsigned> kc_ev_input_channel_changed;
vcs_event_c<void> kc_ev_invalid_device;
vcs_event_c<void> kc_ev_signal_lost;
vcs_event_c<void> kc_ev_signal_gained;
vcs_event_c<void> kc_ev_invalid_signal;
vcs_event_c<void> kc_ev_unrecoverable_error;

vcs_event_c<void> kd_ev_dirty;

vcs_event_c<const resolution_s&> ks_ev_new_output_resolution;
vcs_event_c<const image_s&> ks_ev_new_scaled_image;
vcs_event_c<unsigned> ks_ev_frames_per_second;
vcs_event_c<void> ks_ev_custom_scaler_enabled;
vcs_event_c<void> ks_ev_custom_scaler_disabled;
