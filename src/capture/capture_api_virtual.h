/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef CAPTURE_API_VIRTUAL_H
#define CAPTURE_API_VIRTUAL_H

#include "capture/capture_api.h"

struct capture_api_virtual_s : public capture_api_s
{
    // API overrides.
    bool initialize(void) override;
    bool release(void) override;
    std::string get_device_name(void) const override             { return "Virtual VCS Capture Device"; }
    std::string get_api_name(void) const override                { return "Virtual VCS Capture API"; }
    std::string get_device_driver_version(void) const override   { return "1.0"; }
    std::string get_device_firmware_version(void) const override { return "1.0"; }
    int get_device_max_input_count(void) const override          { return 2; }
    video_signal_parameters_s get_video_signal_parameters(void) const override         { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_default_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_minimum_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_maximum_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    resolution_s get_resolution(void) const override             { return this->defaultResolution; }
    resolution_s get_minimum_resolution(void) const override     { return this->defaultResolution; }
    resolution_s get_maximum_resolution(void) const override     { return this->defaultResolution; }
    unsigned get_refresh_rate(void) const override               { return {}; }
    uint get_missed_frames_count(void) const override            { return 0;  }
    uint get_input_channel_idx(void) const override              { return 0;  }
    uint get_color_depth(void) const override                    { return (unsigned)this->defaultResolution.bpp;  }
    bool is_capturing(void) const override                       { return false; }
    bool has_invalid_signal(void) const override                 { return false; }
    bool has_no_signal(void) const override                      { return false; }
    capture_pixel_format_e get_pixel_format(void) const override { return capture_pixel_format_e::rgb_888; }
    const std::vector<video_signal_parameters_s>& get_mode_params(void) const override { return {}; }
    capture_event_e pop_capture_event_queue(void) override;
    const captured_frame_s& get_frame_buffer(void) override;
    void mark_frame_buffer_as_processed(void) override;

private:
    const resolution_s defaultResolution = resolution_s{640, 480, 32};

    // We don't do any actual capturing, so draw an animating pattern in the
    // frame buffer, instead.
    void animate_frame_buffer(void);

    captured_frame_s frameBuffer;
};

#endif
