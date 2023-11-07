/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/vcs_event/vcs_event.h"

vcs_event_c<const captured_frame_s&> ev_new_captured_frame;
vcs_event_c<const video_mode_s&> ev_new_proposed_video_mode;
vcs_event_c<const video_mode_s&> ev_new_video_mode;
vcs_event_c<unsigned> ev_new_input_channel;
vcs_event_c<void> ev_invalid_capture_device;
vcs_event_c<void> ev_capture_signal_lost;
vcs_event_c<void> ev_capture_signal_gained;
vcs_event_c<void> ev_invalid_capture_signal;
vcs_event_c<void> ev_dirty_output_window;
vcs_event_c<const resolution_s&> ev_new_output_resolution;
vcs_event_c<const image_s&> ev_new_output_image;
vcs_event_c<const refresh_rate_s&> ev_frames_per_second;
vcs_event_c<void> ev_custom_output_scaler_enabled;
vcs_event_c<void> ev_custom_output_scaler_disabled;
vcs_event_c<void> ev_eco_mode_enabled;
vcs_event_c<void> ev_eco_mode_disabled;
vcs_event_c<const captured_frame_s&> ev_frame_processing_finished;
vcs_event_c<const video_preset_s*> ev_video_preset_activated;
vcs_event_c<const video_preset_s*> ev_video_preset_name_changed;
vcs_event_c<unsigned> ev_missed_frames_count;
vcs_event_c<unsigned> ev_capture_processing_latency;
vcs_event_c<const refresh_rate_s&> ev_new_capture_rate;
vcs_event_c<std::vector<const char*>> ev_list_of_supported_video_preset_properties_changed;
vcs_event_c<const video_preset_s*> kc_ev_video_preset_params_changed;
