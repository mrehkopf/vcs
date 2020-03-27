/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Provides a base public interface for interacting with the capture device.
 * 
 * 
 * Usage:
 * 
 *   1. Create a custom capture API by inheriting capture_api_s and implementing
 *      at least its pure virtual functions.
 * 
 *   2. Create an instance of your custom capture API in kc_initialize_capture()
 *      (capture.cpp) and have kc_capture_api() (in capture.cpp) return a reference
 *      to it.
 * 
 *   3. VCS will periodically call pop_capture_event_queue() to get and respond
 *      to the API's capture events.
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

    /*
     * Getters:
     */

    // The number of operable input channels on this device that the API can
    // make use of. The number is expected to reflect consecutive inputs from
    // #1 to n, where n is the value returned.
    virtual int get_device_max_input_count(void) const = 0;

    // A string that describes the device's firmware version. This won't be for
    // any practical use, just for displaying in the GUI and that kind of thing.
    virtual std::string get_device_firmware_version(void) const = 0;

    // A string that describes the device's driver version. This won't be for
    // any practical use, just for displaying in the GUI and that kind of thing.
    virtual std::string get_device_driver_version(void) const = 0;

    // A string that describes the device, e.g. its model name ("Datapath
    // VisionRGB-PRO2", for instance.). This won't be for any practical use,
    // just for displaying in the GUI and that kind of thing.
    virtual std::string get_device_name(void) const = 0;

    // A string that describes the API ("RGBEasy", for instance.). This won't
    // be for any practical use, just for displaying in the GUI and that kind
    // of thing.
    virtual std::string get_api_name(void) const = 0;

    // The device's current video signal parameters.
    virtual video_signal_parameters_s get_video_signal_parameters(void) const = 0;

    // The device's default video signal parameters.
    virtual video_signal_parameters_s get_default_video_signal_parameters(void) const = 0;

    // The lowest value supported by the device for each video signal parameter.
    virtual video_signal_parameters_s get_minimum_video_signal_parameters(void) const = 0;

    // The highest value supported by the device for each video signal
    // parameter.
    virtual video_signal_parameters_s get_maximum_video_signal_parameters(void) const = 0;

    // The device's current capture resolution.
    virtual resolution_s get_resolution(void) const = 0;

    // The lowest capture resolution supported by the device.
    virtual resolution_s get_minimum_resolution(void) const = 0;

    // The highest capture resolution supported by the device.
    virtual resolution_s get_maximum_resolution(void) const = 0;

    // The number of captured frames the VCS thread failed to note between its
    // calls to get_frame_buffer() and mark_frame_buffer_as_processed(). Since
    // the VCS thread locks the API's capture mutex while processing frame data,
    // a missed frame would occur when the API thread receives a new frame
    // while the capture mutex is locked by a non-API thread.
    virtual unsigned get_missed_frames_count(void) const = 0;

    // Get the index of the input channel on which the capture is currently
    // active.
    virtual unsigned get_input_channel_idx(void) const = 0;

    // Get the refresh rate, in Hz, of the current capture signal.
    virtual unsigned get_refresh_rate(void) const = 0;

    // Get the color depth, in bits, of the frames the API currently expects
    // to output via get_frame_buffer(). For instance, for RGB888 this might be
    // 32, and 16 for RGB555 (the latter is technically 15 bits, but its data
    // are probably in 16-bit format).
    //
    // Note: When dealing with individual frames, VCS will refer to the color
    // depth provided by the frame struct rather than this value; but may also
    // verify that both values match before processing the frame further.
    virtual unsigned get_color_depth(void) const = 0;

    // Get the pixel format of the frames the API currently expects to output
    // via get_frame_buffer().
    //
    // Note: When dealing with individual frames, VCS will refer to the pixel
    // format provided by the frame struct rather than this value; but may also
    // verify that both values match before processing the frame further.
    virtual capture_pixel_format_e get_pixel_format(void) const = 0;

    // Returns true if the current capture signal is invalid; false otherwise.
    virtual bool has_invalid_signal(void) const = 0;

    // Returns true if there's currently no capture signal on the selected
    // input channel; false otherwise.
    virtual bool has_no_signal(void) const = 0;

    // Returns true if the device is currently capturing.
    virtual bool is_capturing(void) const = 0;

    // Returns the most recent captured frame's data.
    virtual const captured_frame_s& get_frame_buffer(void) const = 0;

    /*
     * Setters:
     */

    // Used by the VCS thread to notify the API that VCS has finished processing
    // the most recent frame it obtained from get_frame_buffer().
    virtual void mark_frame_buffer_as_processed(void) { return; }

    // Returns the type of what the API considers the most recent and/or most
    // relevant capture event. Capture events that are never returned by this
    // function will be ignored by the VCS thread. This is technically both a
    // getter and a setter, since it gets a capture event, but will likely
    // also result in the API modifying its event queue to remove that event.
    virtual capture_event_e pop_capture_event_queue(void) { return capture_event_e::none; }

    // Ask the device to adopt the given video signal parameters.
    virtual void set_video_signal_parameters(const video_signal_parameters_s &p) { (void)p; return; }

    // Ask the device to adjust the horizontal offset in its current video
    // signal parameters by the given delta. Returns true on success; false
    // otherwise.
    virtual bool adjust_horizontal_offset(const int delta) { (void)delta; return false; }

    // Ask the device to adjust the vertical offset in its current video signal
    // parameters by the given delta. Returns true on success; false otherwise.
    virtual bool adjust_vertical_offset(const int delta) { (void)delta; return false; }

    // Ask the device to change its current input channel to that of the given
    // index. Returns true on success; false otherwise.
    virtual bool set_input_channel(const unsigned idx) { (void)idx; return false; }

    // Ask the device to adopt the given pixel format. Returns true on success;
    // false otherwise.
    virtual bool set_pixel_format(const capture_pixel_format_e pf) { (void)pf; return false; }

    // Ask the device to adopt the given resolution. Returns true on success;
    // false otherwise.
    virtual bool set_resolution(const resolution_s &r) { (void)r; return false; }

    // Ask the API to reset its count of missed frames (cf. get_missed_frames_count()).
    virtual void reset_missed_frames_count(void) { return; }

    // If the capture API runs in a separate thread that modifies the API's
    // state, that thread should lock this mutex while about it; and the main
    // VCS thread will also lock this when accessing that state.
    std::mutex captureMutex;
};

#endif
