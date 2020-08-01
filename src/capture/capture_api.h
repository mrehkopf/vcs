/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

/*! @file
 *
 * @brief
 * The capture API provides an interface for exposing a capture device to VCS.
 * 
 * Usage
 * 
 *   1. Create a new class for your capture device - this will be its specific
 *      capture API. You can look at existing examples: e.g. @a capture_api_virtual.h,
 *      which is for a dummy (null) capture device; and @a capture_api_rgbeasy.h,
 *      which is for the Datapath VisionRGB cards under Windows.
 * 
 *   2. Have the new class inherit from capture_api_s and implement at least
 *      its pure virtual functions.
 * 
 *   3. Inside VCS's @a capture.cpp, instance the new class in kc_initialize_capture(),
 *      and have kc_capture_api() return a reference to that instance.
 * 
 *   4. VCS will now interact with the new capture device though the API; e.g.
 *      by periodically calling kc_capture_api().pop_capture_event_queue() to
 *      see if there are new captured frames to be displayed.
 *
 */

#ifndef CAPTURE_API_H
#define CAPTURE_API_H

#include <string>
#include <mutex>
#include "common/globals.h"
#include "common/refresh_rate.h"
#include "display/display.h"
#include "capture/capture.h"

/*!
 * @brief
 * The base capture API class. Subclass this to add support for new capture
 * devices.
 * 
 * A capture API provides a standard interface for VCS to talk to a capture
 * device. You can thus subclass this base API to add support in VCS for new
 * capture devices.
 * 
 * The main functions here are initialize(), pop_capture_event_queue(), and
 * get_frame_buffer(). VCS uses them to intialize the capture API and to
 * receive new captured frames from it.
 * 
 * When subclassing, you'll need to implement at least the pure virtual
 * functions, which are considered mandatory custom functionality. Other
 * virtual functions can be overridden on a case-by-case basis if their
 * default implementations aren't satisfactory.
 * 
 * For an example of a very basic implementation of a capture API, see
 * @a capture_api_virtual.h. It implements a capture interface for a virtual
 * capture device, i.e. for code that generates images of an animating test
 * pattern.
 */
struct capture_api_s
{
    virtual ~capture_api_s();

    /*!
     * Initializes the capture API and its capture device.
     * 
     * If the call succeeds, the capture API will begin receiving captured
     * frames from the capture device.
     * 
     * In case of error, this function may additionally set the global variable
     * PROGRAM_EXIT_REQUESTED to a truthy value, which will result in VCS's
     * termination once the event loop in main.cpp detects this condition.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @note
     * Regardless of the return value, you should call release() before calling
     * this function again. This ensures that any memory allocated to the
     * capture API is freed.
     * 
     * @see
     * release(), pop_capture_event_queue(), get_frame_buffer()
     */
    virtual bool initialize(void) = 0;

    /*!
     * Releases the capture device and deallocates the capture API's memory
     * buffers.
     * 
     * If the call succeeds, frames will no longer be captured, and the return
     * value from get_frame_buffer() will be undefined until the API is
     * re-initialized with a call to initialize().
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @note
     * Refrain from calling this function without having called initialize()
     * first.
     * 
     * @see
     * initialize()
     */
    virtual bool release(void) = 0;

    /************************
     * Capability queries: */

    /*!
     * Returns @a true if the capture device is capable of capturing from a
     * component video source; @a false otherwise.
     */
    virtual bool device_supports_component_capture(void) const { return false; }

    /*!
     * Returns @a true if the capture device is capable of capturing from a
     * composite video source; @a false otherwise.
     */
    virtual bool device_supports_composite_capture(void) const { return false; }

    /*!
     * Returns @a true if the capture device supports hardware de-interlacing;
     * @a false otherwise.
     */
    virtual bool device_supports_deinterlacing(void) const { return false; }

    /*!
     * Returns @a true if the capture device is capable of capturing from an
     * S-Video source; @a false otherwise.
     */
    virtual bool device_supports_svideo(void) const { return false; }
    
    /*!
     * Returns @a true if the capture device is capable of streaming frames via
     * direct memory access (DMA); @a false otherwise.
     */
    virtual bool device_supports_dma(void) const { return false; }

    /*!
     * Returns @a true if the capture device is capable of capturing from a
     * digital (DVI) source; @a false otherwise.
     */
    virtual bool device_supports_dvi(void) const { return false; }

    /*!
     * Returns @a true if the capture device is capable of capturing from an
     * analog (VGA) source; @a false otherwise.
     */
    virtual bool device_supports_vga(void) const { return false; }

    /*!
     * Returns @a true if the capture device is capable of capturing in YUV
     * color; @a false otherwise.
     */
    virtual bool device_supports_yuv(void) const { return false; }

    /*************
     * Getters: */

    /*!
     * Returns the number of input channels on the capture device that're
     * available to the capture API.
     * 
     * The value returned is an integer in the range [1,n] such that if the
     * capture device has, for instance, 16 input channels and the capture API
     * can use two of them, 2 is returned.
     * 
     * @see
     * get_input_channel_idx()
     */
    virtual int get_device_maximum_input_count(void) const = 0;

    /*!
     * Returns a string that identifies the capture device's firmware version;
     * e.g. "14.12.3".
     * 
     * Will return "Unknown" if the firmware version is not known.
     * 
     * @see
     * get_device_driver_version()
     */
    virtual std::string get_device_firmware_version(void) const = 0;

    /*!
     * Returns a string that identifies the capture device's driver version;
     * e.g. "14.12.3".
     * 
     * Will return "Unknown" if the firmware version is not known.
     * 
     * @see
     * get_device_firmware_version()
     */
    virtual std::string get_device_driver_version(void) const = 0;

    /*!
     * Returns a string that identifies the capture device; e.g. "Datapath
     * VisionRGB-PRO2".
     * 
     * Will return "Unknown" if the capture device is not recognized.
     */
    virtual std::string get_device_name(void) const = 0;

    /*!
     * Returns a string that identifies the capture API; e.g. "RGBEasy".
     */
    virtual std::string get_api_name(void) const = 0;

    /*!
     * Returns the capture device's current video signal parameters.
     * 
     * @see
     * set_video_signal_parameters(), get_minimum_video_signal_parameters(),
     * get_maximum_video_signal_parameters(), get_default_video_signal_parameters()
     */
    virtual video_signal_parameters_s get_video_signal_parameters(void) const = 0;

    /*!
     * Returns the capture device's default video signal parameters.
     * 
     * @see
     * get_video_signal_parameters(), get_video_signal_parameters(),
     * get_minimum_video_signal_parameters(), get_maximum_video_signal_parameters()
     */
    virtual video_signal_parameters_s get_default_video_signal_parameters(void) const = 0;

    /*!
     * Returns the minimum value supported by the capture device for each video
     * signal parameter.
     * 
     * @see
     * set_video_signal_parameters(), get_video_signal_parameters(),
     * get_maximum_video_signal_parameters(), get_default_video_signal_parameters()
     */
    virtual video_signal_parameters_s get_minimum_video_signal_parameters(void) const = 0;

    /*!
     * Returns the maximum value supported by the capture device for each video
     * signal parameter.
     * 
     * @see
     * set_video_signal_parameters(), get_video_signal_parameters(),
     * get_minimum_video_signal_parameters(), get_default_video_signal_parameters()
     */
    virtual video_signal_parameters_s get_maximum_video_signal_parameters(void) const = 0;

    /*!
     * Returns the capture device's current input/output resolution.
     * 
     * @note
     * The capture device's input and output resolutions are expected to always
     * be equal.
     * 
     * @warning
     * Don't take this to be the resolution of the latest captured frame
     * (returned from get_frame_buffer()), as the capture device's resolution
     * may have changed since that frame was captured.
     * 
     * @see
     * set_resolution(), get_minimum_resolution(), get_maximum_resolution()
     */
    virtual resolution_s get_resolution(void) const = 0;

    /*!
     * Returns the minimum capture resolution supported by the capture API.
     * 
     * @note
     * This resolution may be larger - but not smaller - than the minimum
     * input/output resolution supported by the capture device.
     * 
     * @see
     * get_maximum_resolution(), get_resolution(), set_resolution()
     */
    virtual resolution_s get_minimum_resolution(void) const = 0;

    /*!
     * Returns the maximum capture resolution supported by the capture API.
     * 
     * @note
     * This resolution may be smaller - but not larger - than the maximum
     * input/output resolution supported by the capture device.
     * 
     * @see
     * get_minimum_resolution(), get_resolution(), set_resolution()
     */
    virtual resolution_s get_maximum_resolution(void) const = 0;

    /*!
     * Returns the difference between the number of frames received by the
     * capture API from the capture device and the number of times
     * mark_frame_buffer_as_processed() has been called.
     * 
     * If this value is above 0, it indicates that VCS is failing to process
     * and display captured frames as fast as the capture device is producing
     * them. This could be a symptom of e.g. an inadequately performant host
     * CPU.
     * 
     * @see
     * reset_missed_frames_count()
     */
    virtual unsigned get_missed_frames_count(void) const = 0;

    /*!
     * Returns the index value of the capture API's input channel on which the
     * capture device is currently listening for signals. The value is in the
     * range [0,n-1], where n = get_device_maximum_input_count().
     * 
     * If the capture device has more input channels than are supported by the
     * API, the API is expected to map the index to a consecutive range. For
     * example, if channels #1, #5, and #6 on the capture device are available
     * to the API, capturing on channel #5 would correspond to an index value
     * of 1, with index 0 being channel #1 and index 2 channel #6.
     * 
     * @see
     * get_device_maximum_input_count()
     */
    virtual unsigned get_input_channel_idx(void) const = 0;

    /*!
     * Returns the refresh rate of the current capture signal.
     */
    virtual refresh_rate_s get_refresh_rate(void) const = 0;

    /*!
     * Returns the color depth, in bits, that the capture API currently expects
     * to receive captured frames in.
     * 
     * For RGB888 frames the color depth would be 32; 16 for RGB565 frames; etc.
     * 
     * The color depth of a given frame as returned from get_frame_buffer() may
     * be different from this value e.g. if the capture color depth was changed
     * just after the frame was captured.
     */
    virtual unsigned get_color_depth(void) const = 0;

    /*!
     * Returns the pixel format that the capture device is currently storing
     * its captured frames in.
     *
     * The pixel format of a given frame as returned from get_frame_buffer() may
     * be different from this value e.g. if the capture pixel format was changed
     * just after the frame was captured.
     */
    virtual capture_pixel_format_e get_pixel_format(void) const = 0;

    /*!
     * Returns @a true if the current capture signal is invalid; @a false
     * otherwise.
     * 
     * @see
     * has_no_signal()
     */
    virtual bool has_invalid_signal(void) const = 0;

    /*!
     * Returns @a true if there's currently no capture signal on the capture
     * device's active input channel; @a false otherwise.
     * 
     * @see
     * get_input_channel_idx(), set_input_channel()
     */
    virtual bool has_no_signal(void) const = 0;

    /*!
     * Returns @a true if the device is currently capturing; @a false otherwise.
     */
    virtual bool is_capturing(void) const = 0;

    /*!
     * Returns the most recently-captured frame's data.
     * 
     * @warning
     * The caller should lock @ref captureMutex while accessing the returned
     * data. Failure to do so may result in a race condition, as the capture
     * API is permitted to operate on these data outside of the caller's
     * thread.
     */
    virtual const captured_frame_s &get_frame_buffer(void) const = 0;

    /*************
     * Setters: */

    /*!
     * Called by VCS to notify the capture API that it has finished processing
     * the latest frame obtained via get_frame_buffer(). The API is then free
     * to e.g. overwrite the frame's data.
     * 
     * Returns @a true on success; @a false otherwise.
     */
    virtual bool mark_frame_buffer_as_processed(void) { return false; }

    /*!
     * Returns the latest capture event and removes it from the capture API's
     * event queue. The caller can then respond to the event; e.g. by calling
     * get_frame_buffer() if the event is a new frame.
     */
    virtual capture_event_e pop_capture_event_queue(void) { return capture_event_e::none; }

    /*!
     * Assigns to the capture device the given video signal parameters.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @see
     * get_video_signal_parameters(), get_minimum_video_signal_parameters(),
     * get_maximum_video_signal_parameters(), get_default_video_signal_parameters()
     */
    virtual bool set_video_signal_parameters(const video_signal_parameters_s &p) { (void)p; return true; }

    /*!
     * Tells the capture device to start listening for signals on the given
     * input channel.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @see
     * get_input_channel_idx(), get_device_maximum_input_count()
     */
    virtual bool set_input_channel(const unsigned idx) { (void)idx; return false; }

    /*!
     * Tells the capture device to store its captured frames using the given
     * pixel format.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @see
     * get_pixel_format()
     */
    virtual bool set_pixel_format(const capture_pixel_format_e pf) { (void)pf; return false; }

    /*!
     * Tells the capture device to adopt the given resolution as its input and
     * output resolution.
     * 
     * @note
     * The capture device must adopt this as both its input and output
     * resolution. For example, if this resolution is 800 x 600, the capture
     * device should interpret the video signal as if it were 800 x 600, rather
     * than upscaling or downscaling the frame to 800 x 600 after capturing.
     * 
     * @warning
     * Since this will be set as the capture device's input resolution,
     * captured frames may exhibit artefacting if the resolution doesn't match
     * the video signal's true or intended resolution.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @see
     * get_resolution(), get_minimum_resolution(), get_maximum_resolution()
     */
    virtual bool set_resolution(const resolution_s &r) { (void)r; return false; }

    /*!
     * Tells the capture API to reset its count of missed frames.
     * 
     * Returns @a true on success; @a false otherwise.
     * 
     * @see
     * get_missed_frames_count()
     */
    virtual bool reset_missed_frames_count(void) { return false; }

    /*!
     * A mutex used to coordinate access to the capture API's data between the
     * threads of the capture API, VCS, and the capture device.
     * 
     * The VCS thread will lock this mutex while it's accessing frame data
     * received from get_frame_buffer(). During this time, the capture API
     * should not modify those data nor allow the capture device to modify
     * them.
     */
    std::mutex captureMutex;
};

#endif
