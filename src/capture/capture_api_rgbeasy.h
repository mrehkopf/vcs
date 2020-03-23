/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a public interface for interacting with the capture device.
 *
 */

#ifndef CAPTURE_API_RGBEASY_H
#define CAPTURE_API_RGBEASY_H

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
    bool initialize(void) override;
    bool release(void) override;

    // Capture device/API capabilities.
    bool device_supports_component_capture(void) const override;
    bool device_supports_composite_capture(void) const override;
    bool device_supports_deinterlacing(void) const override;
    bool device_supports_svideo(void) const override;
    bool device_supports_dma(void) const override;
    bool device_supports_dvi(void) const override;
    bool device_supports_vga(void) const override;
    bool device_supports_yuv(void) const override;

    // Getters.
    std::string              get_device_firmware_version(void) const override;
    std::string              get_device_driver_version(void)   const override;
    std::string              get_device_name(void)             const override;
    std::string              get_api_name(void)                const override;
    int                      get_maximum_input_count(void)     const override;
    capture_color_settings_s get_color_settings(void)          const override;
    capture_color_settings_s get_default_color_settings(void)  const override;
    capture_color_settings_s get_minimum_color_settings(void)  const override;
    capture_color_settings_s get_maximum_color_settings(void)  const override;
    capture_video_settings_s get_video_settings(void)          const override;
    capture_video_settings_s get_default_video_settings(void)  const override;
    capture_video_settings_s get_minimum_video_settings(void)  const override;
    capture_video_settings_s get_maximum_video_settings(void)  const override;
    resolution_s             get_resolution(void)              const override;
    resolution_s             get_minimum_resolution(void)      const override;
    resolution_s             get_maximum_resolution(void)      const override;
    const captured_frame_s&  get_frame_buffer(void)            override;
    capture_event_e          get_latest_capture_event(void)    override;
    capture_signal_s         get_signal_info(void)             const override;
    int                      get_frame_rate(void)              const override;

    uint get_num_missed_frames(void) override;
    uint get_input_channel_idx(void) override;
    uint get_input_color_depth(void) override;
    bool get_are_frames_being_dropped(void) override;
    bool get_is_capture_active(void) override;
    bool get_should_current_frame_be_skipped(void) override;
    bool get_is_invalid_signal(void) override;
    bool get_no_signal(void) override;
    capture_pixel_format_e get_pixel_format(void) override;
    const std::vector<video_mode_params_s>& get_mode_params(void) override;
    video_mode_params_s get_mode_params_for_resolution(const resolution_s r) override;

    // Setters.
    void set_mode_params(const std::vector<video_mode_params_s> &modeParams) override;
    bool set_mode_parameters_for_resolution(const resolution_s r) override;
    void set_color_settings(const capture_color_settings_s c) override;
    void set_video_settings(const capture_video_settings_s v) override;
    bool adjust_video_horizontal_offset(const int delta) override;
    bool adjust_video_vertical_offset(const int delta) override;
    void report_frame_buffer_processing_finished(void) override;
    bool set_input_channel(const unsigned channel) override;
    bool set_input_color_depth(const unsigned bpp) override;
    bool set_frame_dropping(const unsigned drop) override;
    bool set_resolution(const resolution_s &r) override;
    void apply_new_capture_resolution(void) override;
    void reset_missed_frames_count(void) override;

private:
    // Returns true if the given RGBEasy API call return value indicates an error.
    bool apicall_succeeded(long callReturnValue) const;

    void update_known_video_mode_params(const resolution_s r,
                                        const capture_color_settings_s *const c,
                                        const capture_video_settings_s *const v);

    bool initialize_hardware(void);

    bool start_capture(void);

    bool stop_capture(void);

    bool release_hardware(void);

    // Converts VCS's pixel format into the RGBEasy pixel format.
    PIXELFORMAT pixel_format_to_rgbeasy_pixel_format(capture_pixel_format_e fmt);
};

#endif
