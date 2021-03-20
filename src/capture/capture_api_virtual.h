/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIRTUAL

#ifndef VCS_CAPTURE_CAPTURE_API_VIRTUAL_H
#define VCS_CAPTURE_CAPTURE_API_VIRTUAL_H

#include "capture/capture_api.h"
#include "common/globals.h"

struct capture_api_virtual_s : public capture_api_s
{
    // API overrides.
    bool initialize(void) override;
    bool release(void) override;
    std::string get_device_name(void) const override             { return "VCS Virtual Capture Device"; }
    std::string get_api_name(void) const override                { return "VCS Virtual Capture API";    }
    std::string get_device_driver_version(void) const override   { return "1.0"; }
    std::string get_device_firmware_version(void) const override { return "1.0"; }
    int get_device_maximum_input_count(void) const override      { return 2;     }
    video_signal_parameters_s get_video_signal_parameters(void) const override         { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_default_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_minimum_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    video_signal_parameters_s get_maximum_video_signal_parameters(void) const override { return video_signal_parameters_s{}; }
    bool set_resolution(const resolution_s &r) override;
    resolution_s get_resolution(void) const override             { return this->frameBuffer.r; }
    resolution_s get_minimum_resolution(void) const override     { return this->minResolution; }
    resolution_s get_maximum_resolution(void) const override     { return this->maxResolution; }
    refresh_rate_s get_refresh_rate(void) const override;
    uint get_missed_frames_count(void) const override            { return 0; }
    uint get_input_channel_idx(void) const override              { return this->inputChannelIdx; }
    bool set_input_channel(const unsigned idx) override;
    uint get_color_depth(void) const override                    { return (unsigned)this->frameBuffer.r.bpp; }
    bool is_capturing(void) const override                       { return false; }
    bool has_invalid_signal(void) const override                 { return false; }
    bool has_invalid_device(void) const override                 { return false; }
    bool has_no_signal(void) const override                      { return false; }
    capture_pixel_format_e get_pixel_format(void) const override { return this->defaultPixelFormat; }
    capture_event_e pop_capture_event_queue(void) override;
    const captured_frame_s& get_frame_buffer(void) const override;

    bool device_supports_component_capture(void) const override  { return true; }
    bool device_supports_composite_capture(void) const override  { return true; }
    bool device_supports_deinterlacing(void) const override      { return true; }
    bool device_supports_svideo(void) const override             { return true; }
    bool device_supports_dma(void) const override                { return true; }
    bool device_supports_dvi(void) const override                { return true; }
    bool device_supports_vga(void) const override                { return true; }
    bool device_supports_yuv(void) const override                { return true; }

private:
    const resolution_s maxResolution = resolution_s{MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT, 32};
    const resolution_s minResolution = resolution_s{320, 200, 32};
    const capture_pixel_format_e defaultPixelFormat = capture_pixel_format_e::rgb_888;

    captured_frame_s frameBuffer;

    unsigned inputChannelIdx = 0;
};

#endif

#endif
