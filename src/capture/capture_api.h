/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a public interface for interacting with the capture device.
 *
 */

#ifndef CAPTURE_API_H
#define CAPTURE_API_H

#include <string>
#include "common/globals.h"
#include "display/display.h"
#include "capture/capture.h"

struct capture_api_s
{
    virtual ~capture_api_s();

    // Initializes the API and the capture device, and tells the device to begin
    // capturing.
    virtual bool initialize(void) = 0;

    // Stops capturing, releases the capture device and deallocates the API.
    virtual bool release(void) = 0;

    // Query capture device/API capabilities.
    virtual bool device_supports_component_capture(void) const { return false; }
    virtual bool device_supports_composite_capture(void) const { return false; }
    virtual bool device_supports_deinterlacing(void)     const { return false; }
    virtual bool device_supports_svideo(void)            const { return false; }
    virtual bool device_supports_dma(void)               const { return false; }
    virtual bool device_supports_dvi(void)               const { return false; }
    virtual bool device_supports_vga(void)               const { return false; }
    virtual bool device_supports_yuv(void)               const { return false; }

    // Getters.
    virtual int                      get_device_max_input_count(void)  const = 0;
    virtual std::string              get_device_firmware_version(void) const = 0;
    virtual std::string              get_device_driver_version(void)   const = 0;
    virtual std::string              get_device_name(void)             const = 0;
    virtual std::string              get_api_name(void)                const = 0;
    virtual capture_color_settings_s get_color_settings(void)          const = 0;
    virtual capture_color_settings_s get_default_color_settings(void)  const = 0;
    virtual capture_color_settings_s get_minimum_color_settings(void)  const = 0;
    virtual capture_color_settings_s get_maximum_color_settings(void)  const = 0;
    virtual capture_video_settings_s get_video_settings(void)          const = 0;
    virtual capture_video_settings_s get_default_video_settings(void)  const = 0;
    virtual capture_video_settings_s get_minimum_video_settings(void)  const = 0;
    virtual capture_video_settings_s get_maximum_video_settings(void)  const = 0;
    virtual resolution_s             get_resolution(void)              const = 0;
    virtual resolution_s             get_minimum_resolution(void)      const = 0;
    virtual resolution_s             get_maximum_resolution(void)      const = 0;
    virtual capture_signal_s         get_signal_info(void)             const = 0;
    virtual capture_event_e          pop_capture_event_queue(void)     = 0;

    // Miscellaneous getters; TODO.
    virtual uint get_num_missed_frames(void)                                         = 0;
    virtual uint get_input_channel_idx(void)                                         = 0;
    virtual uint get_input_color_depth(void)                                         = 0;
    virtual bool get_are_frames_being_dropped(void)                                  = 0;
    virtual bool get_is_capture_active(void)                                         = 0;
    virtual bool get_should_current_frame_be_skipped(void)                           = 0;
    virtual bool get_is_invalid_signal(void)                                         = 0;
    virtual bool get_no_signal(void)                                                 = 0;
    virtual capturePixelFormat_e get_pixel_format(void)                            = 0;
    virtual const std::vector<video_mode_params_s>& get_mode_params(void)            = 0;
    virtual video_mode_params_s get_mode_params_for_resolution(const resolution_s r) = 0;

    // Setters.
    virtual void set_mode_params(const std::vector<video_mode_params_s> &modeParams) { (void)modeParams; return;       }
    virtual bool set_mode_parameters_for_resolution(const resolution_s r)            { (void)r;          return false; }
    virtual void set_color_settings(const capture_color_settings_s c)                { (void)c;          return;       }
    virtual void set_video_settings(const capture_video_settings_s v)                { (void)v;          return;       }
    virtual bool adjust_video_horizontal_offset(const int delta)                     { (void)delta;      return false; }
    virtual bool adjust_video_vertical_offset(const int delta)                       { (void)delta;      return false; }
    virtual bool set_input_channel(const unsigned channel)                           { (void)channel;    return false; }
    virtual bool set_input_color_depth(const unsigned bpp)                           { (void)bpp;        return false; }
    virtual bool set_frame_dropping(const unsigned drop)                             { (void)drop;       return false; }
    virtual void apply_new_capture_resolution(void)                                  {                   return;       }
    virtual void reset_missed_frames_count(void)                                     {                   return;       }
    virtual bool set_resolution(const resolution_s &r)                               { (void)r;          return false; }
    virtual const captured_frame_s& reserve_frame_buffer(void)                       = 0;
    virtual void unreserve_frame_buffer(void)                                        = 0;
};

#endif
