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
#include <mutex>
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
    virtual uint                      get_missed_frames_count(void)             const = 0;
    virtual uint                      get_input_channel_idx(void)               const = 0;
    virtual unsigned                  get_refresh_rate(void)                    const = 0;
    virtual uint                      get_color_depth(void)                     const = 0;
    virtual capture_pixel_format_e    get_pixel_format(void)                    const = 0;
    virtual bool                      has_invalid_signal(void)                  const = 0;
    virtual bool                      has_no_signal(void)                       const = 0;
    virtual bool                      is_capturing(void)                        const = 0;
    virtual const captured_frame_s&   get_frame_buffer(void)                    const = 0;

    // Setters.
    virtual void            mark_frame_buffer_as_processed(void)                            { return;                       }
    virtual capture_event_e pop_capture_event_queue(void)                                   { return capture_event_e::none; }
    virtual void            set_video_signal_parameters(const video_signal_parameters_s &p) { (void)p;     return;          }
    virtual bool            adjust_horizontal_offset(const int delta)                       { (void)delta; return false;    }
    virtual bool            adjust_vertical_offset(const int delta)                         { (void)delta; return false;    }
    virtual bool            set_input_channel(const unsigned idx)                           { (void)idx;   return false;    }
    virtual bool            set_pixel_format(const capture_pixel_format_e pf)               { (void)pf;    return false;    }
    virtual void            reset_missed_frames_count(void)                                 { return;                       }
    virtual bool            set_resolution(const resolution_s &r)                           { (void)r;     return false;    }

    // If the capture API runs in a separate thread that modifies the API's
    // state, that thread should lock this mutex while about it; and the main
    // VCS thread will also lock this when accessing that state.
    std::mutex captureMutex;
};

#endif
