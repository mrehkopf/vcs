/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef CAPTURE_API_RGBEASY_H
#define CAPTURE_API_RGBEASY_H

#include <mutex>
#include "capture/capture_api.h"

#if _WIN32
    #include <windows.h>
    #include <rgb.h>
    #include <rgbapi.h>
    #include <rgberror.h>
#else
    #include "capture/null_rgbeasy.h"
#endif

struct capture_api_rgbeasy_s : public capture_api_s
{
    // Convenience getters for the RGBEasy thread to call.
    HRGB rgbeasy_capture_handle(void);
    void push_capture_event(capture_event_e event);

    // API overrides.
    bool initialize(void) override;
    bool release(void) override;
    bool device_supports_component_capture(void) const override;
    bool device_supports_composite_capture(void) const override;
    bool device_supports_deinterlacing(void) const override;
    bool device_supports_svideo(void) const override;
    bool device_supports_dma(void) const override;
    bool device_supports_dvi(void) const override;
    bool device_supports_vga(void) const override;
    bool device_supports_yuv(void) const override;
    std::string get_device_firmware_version(void) const override;
    std::string get_device_driver_version(void) const override;
    std::string get_device_name(void) const override;
    std::string  get_api_name(void) const override;
    int get_device_max_input_count(void)  const override;
    video_signal_parameters_s get_video_signal_parameters(void) const override;
    video_signal_parameters_s get_default_video_signal_parameters(void) const override;
    video_signal_parameters_s get_minimum_video_signal_parameters(void) const override;
    video_signal_parameters_s get_maximum_video_signal_parameters(void) const override;
    resolution_s get_resolution(void) const override;
    resolution_s get_minimum_resolution(void) const override;
    resolution_s get_maximum_resolution(void) const override;
    unsigned get_refresh_rate(void) const override;
    uint get_missed_frames_count(void) const override;
    uint get_input_channel_idx(void) const override;
    uint get_color_depth(void) const override;
    bool is_capturing(void) const override;
    bool has_invalid_signal(void) const override;
    bool has_no_signal(void) const override;
    capture_pixel_format_e get_pixel_format(void) const override;
    const captured_frame_s& get_frame_buffer(void) const override;
    void mark_frame_buffer_as_processed(void) override;
    capture_event_e pop_capture_event_queue(void) override;
    virtual void set_video_signal_parameters(const video_signal_parameters_s p) override;
    bool adjust_horizontal_offset(const int delta) override;
    bool adjust_vertical_offset(const int delta) override;
    bool set_input_channel(const unsigned idx) override;
    bool set_pixel_format(const capture_pixel_format_e pf) override;
    bool set_resolution(const resolution_s &r) override;
    void reset_missed_frames_count(void) override;

private:
    // Returns true if the given RGBEasy API call return value indicates an error.
    bool apicall_succeeded(long callReturnValue) const;

    void update_known_video_signal_parameters(const resolution_s r, const video_signal_parameters_s &p);

    video_signal_parameters_s get_video_signal_parameters_for_resolution(const resolution_s r) const;

    bool initialize_hardware(void);

    bool start_capture(void);

    bool stop_capture(void);

    bool release_hardware(void);

    bool pop_capture_event(capture_event_e event);

    bool assign_video_signal_params_for_resolution(const resolution_s r);

    // Converts VCS's pixel format into the RGBEasy pixel format.
    PIXELFORMAT pixel_format_to_rgbeasy_pixel_format(capture_pixel_format_e fmt);
    
    HRGB captureHandle = 0;

    HRGBDLL rgbAPIHandle = 0;

    // Set to 1 if we're currently capturing.
    bool captureIsActive = false;

    // The corresponding flag will be set to true when the RGBEasy callback
    // thread notifies us of such an event; and reset back to false when we've
    // handled that event.
    bool rgbeasyCaptureEventFlags[static_cast<int>(capture_event_e::num_enumerators)] = {false};
};

#endif
