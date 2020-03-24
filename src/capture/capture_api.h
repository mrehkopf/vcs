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
    virtual int                       get_device_max_input_count(void)          const = 0;
    virtual std::string               get_device_firmware_version(void)         const = 0;
    virtual std::string               get_device_driver_version(void)           const = 0;
    virtual std::string               get_device_name(void)                     const = 0;
    virtual std::string               get_api_name(void)                        const = 0;
    virtual video_signal_parameters_s get_video_signal_parameters(void)         const = 0;
    virtual video_signal_parameters_s get_default_video_signal_parameters(void) const = 0;
    virtual video_signal_parameters_s get_minimum_video_signal_parameters(void) const = 0;
    virtual video_signal_parameters_s get_maximum_video_signal_parameters(void) const = 0;
    virtual resolution_s              get_resolution(void)                      const = 0;
    virtual resolution_s              get_minimum_resolution(void)              const = 0;
    virtual resolution_s              get_maximum_resolution(void)              const = 0;
    virtual capture_signal_s          get_signal_info(void)                     const = 0;
    virtual uint                      get_missed_frames_count(void)             const = 0;
    virtual uint                      get_input_channel_idx(void)               const = 0;
    virtual uint                      get_color_depth(void)                     const = 0;
    virtual bool                      are_frames_being_dropped(void)            const = 0;
    virtual bool                      is_capture_active(void)                   const = 0;
    virtual bool                      should_current_frame_be_skipped(void)     const = 0;
    virtual bool                      is_signal_invalid(void)                   const = 0;
    virtual bool                      no_signal(void)                           const = 0;
    virtual capture_pixel_format_e    get_pixel_format(void)                    const = 0;
    virtual const std::vector<video_signal_parameters_s>& get_mode_params(void) const = 0;

    // Setters.
    virtual const captured_frame_s& reserve_frame_buffer(void)                                     = 0;
    virtual void                    unreserve_frame_buffer(void)                                   = 0;
    virtual capture_event_e         pop_capture_event_queue(void)                                  = 0;
    virtual void                    assign_video_signal_parameter_sets(const std::vector<video_signal_parameters_s> &p) { (void)p; return; }
    virtual void                    set_video_signal_parameters(const video_signal_parameters_s p) { (void)p;      return;       }
    virtual bool                    adjust_horizontal_offset(const int delta)                      { (void)delta;  return false; }
    virtual bool                    adjust_vertical_offset(const int delta)                        { (void)delta;  return false; }
    virtual bool                    set_input_channel(const unsigned idx)                          { (void)idx;    return false; }
    virtual bool                    set_color_depth(const unsigned bpp)                            { (void)bpp;    return false; }
    virtual void                    apply_new_capture_resolution(void)                             {               return;       }
    virtual void                    reset_missed_frames_count(void)                                {               return;       }
    virtual bool                    set_resolution(const resolution_s &r)                          { (void)r;      return false; }
};

#endif
