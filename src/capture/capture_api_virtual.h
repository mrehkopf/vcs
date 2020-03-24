/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a public interface for interacting with the capture device.
 *
 */

#ifndef CAPTURE_API_VIRTUAL_H
#define CAPTURE_API_VIRTUAL_H

#include "capture/capture_api.h"

struct capture_api_virtual_s : public capture_api_s
{
    bool initialize(void) override;
    bool release(void) override;

    // Getters.
    std::string              get_device_name(void)             const override { return "Virtual VCS Capture Device"; }
    std::string              get_api_name(void)                const override { return "Virtual VCS Capture API"; }
    std::string              get_device_driver_version(void)   const override { return "1.0"; }
    std::string              get_device_firmware_version(void) const override { return "1.0"; }
    int                      get_maximum_input_count(void)     const override { return 2; }
    capture_color_settings_s get_default_color_settings(void)  const override { return capture_color_settings_s{}; }
    capture_color_settings_s get_minimum_color_settings(void)  const override { return capture_color_settings_s{}; }
    capture_color_settings_s get_maximum_color_settings(void)  const override { return capture_color_settings_s{}; }
    capture_video_settings_s get_default_video_settings(void)  const override { return capture_video_settings_s{}; }
    capture_video_settings_s get_minimum_video_settings(void)  const override { return capture_video_settings_s{}; }
    capture_video_settings_s get_maximum_video_settings(void)  const override { return capture_video_settings_s{}; }
    resolution_s             get_resolution(void)              const override { return this->defaultResolution; }
    resolution_s             get_minimum_resolution(void)      const override { return this->defaultResolution; }
    resolution_s             get_maximum_resolution(void)      const override { return this->defaultResolution; }
    capture_color_settings_s get_color_settings(void)          const override { return {}; }
    capture_video_settings_s get_video_settings(void)          const override { return {}; }
    capture_signal_s         get_signal_info(void)             const override { return {}; }
    int                      get_frame_rate(void)              const override { return 0; }
    capture_event_e          pop_capture_event_queue(void)    override;

    // Miscellaneous getters; TODO.
    uint get_num_missed_frames(void)                                         override { return 0;              }
    uint get_input_channel_idx(void)                                         override { return 0;              }
    uint get_input_color_depth(void)                                         override { return 0;              }
    bool get_are_frames_being_dropped(void)                                  override { return false;          }
    bool get_is_capture_active(void)                                         override { return false;          }
    bool get_should_current_frame_be_skipped(void)                           override { return false;          }
    bool get_is_invalid_signal(void)                                         override { return false;          }
    bool get_no_signal(void)                                                 override { return false;          }
    capture_pixel_format_e get_pixel_format(void)                            override { return capture_pixel_format_e::rgb_888; }
    const std::vector<video_mode_params_s>& get_mode_params(void)            override { return {};             }
    video_mode_params_s get_mode_params_for_resolution(const resolution_s r) override { (void)r; return {};    }

    // Setters.
    const captured_frame_s& reserve_frame_buffer(void) override;
    void unreserve_frame_buffer(void) override;

private:
    const resolution_s defaultResolution = resolution_s{640, 480, 32};

    // We don't do any actual capturing, so draw an animating pattern in the
    // frame buffer, instead.
    void animate_frame_buffer(void);

    captured_frame_s frameBuffer;
};

#endif
