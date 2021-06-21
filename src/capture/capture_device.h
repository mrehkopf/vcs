/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

/*! @file
 *
 * @brief
 * An interface for exposing capture devices to VCS's capture subsystem.
 * 
 * Usage:
 * 
 *   1. Subclass capture_device_s for your capture device. You can find examples
 *      in VCS's existing implementations: e.g. @a capture_device_virtual.h
 *      (generates a test pattern without interacting with any capture device)
 *      or @a capture_device_rgbeasy.h (interfaces with Datapath VisionRGB
 *      cards under Windows).
 * 
 *   2. The subclass must implement at least the pure virtual functions of
 *      capture_device_s.
 * 
 *   3. Inside kc_initialize_capture() in @a capture.cpp, instance the subclass,
 *      and have kc_capture_device() return a reference to that instance.
 * 
 *   4. VCS will now interact with the new capture device though the interface;
 *      e.g. by periodically calling kc_capture_device().pop_capture_event_queue()
 *      to see if there are new captured frames to be displayed.
 *
 */

#ifndef VCS_CAPTURE_CAPTURE_SOURCE_H
#define VCS_CAPTURE_CAPTURE_SOURCE_H

#include <string>
#include <mutex>
#include "common/globals.h"
#include "common/refresh_rate.h"
#include "display/display.h"
#include "capture/capture.h"

/*!
 * @brief
 * The base capture device interface. Subclass this to add support for new capture
 * devices in VCS.
 * 
 * The capture device interface provides an abstract interface for VCS to
 * interact with capture devices.
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
 * For a basic example of a subclass of this interface, see e.g.
 * @a capture_device_virtual.h. It implements a virtual device for generating
 * test patterns without the presence of an actual capture device.
 */
struct capture_device_s
{
    virtual ~capture_device_s();

    /*!
     * Initializes the interface and the associated capture device.
     * 
     * If the call succeeds, the interface will begin receiving captured frames
     * from the device.
     * 
     * In case of error, this function may additionally set the global variable
     * PROGRAM_EXIT_REQUESTED to a truthy value, which will result in VCS's
     * termination once the event loop in @a main.cpp detects this condition.
     * 
     * Returns true on success; false otherwise.
     * 
     * @note
     * If you're going to call this function more than once during the program's
     * execution, you'll need to manually invoke release() before each subsequent
     * call, or a resource leak may occur. VCS will make the final call to
     * release() on program termination.
     * 
     * @see
     * release(), pop_capture_event_queue(), get_frame_buffer()
     */
    virtual bool initialize(void) = 0;

    /*!
     * Releases the capture device and deallocates the interface's memory buffers.
     * 
     * If the call succeeds, frames will no longer be captured, and the return
     * value from get_frame_buffer() will be undefined until the interface is
     * re-initialized with a call to initialize().
     * 
     * Returns true on success; false otherwise.
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
     * Returns true if the capture device is capable of capturing from a
     * component video source; false otherwise.
     */
    virtual bool device_supports_component_capture(void) const { return false; }

    /*!
     * Returns true if the capture device is capable of capturing from a
     * composite video source; false otherwise.
     */
    virtual bool device_supports_composite_capture(void) const { return false; }

    /*!
     * Returns true if the capture device supports hardware de-interlacing;
     * false otherwise.
     */
    virtual bool device_supports_deinterlacing(void) const { return false; }

    /*!
     * Returns true if the capture device is capable of capturing from an
     * S-Video source; false otherwise.
     */
    virtual bool device_supports_svideo(void) const { return false; }
    
    /*!
     * Returns true if the capture device is capable of streaming frames via
     * direct memory access (DMA); false otherwise.
     */
    virtual bool device_supports_dma(void) const { return false; }

    /*!
     * Returns true if the capture device is capable of capturing from a
     * digital (DVI) source; false otherwise.
     */
    virtual bool device_supports_dvi(void) const { return false; }

    /*!
     * Returns true if the capture device is capable of capturing from an
     * analog (VGA) source; false otherwise.
     */
    virtual bool device_supports_vga(void) const { return false; }

    /*!
     * Returns true if the capture device is capable of capturing in YUV
     * color; false otherwise.
     */
    virtual bool device_supports_yuv(void) const { return false; }

    /*************
     * Getters: */

    /*!
     * Returns the number of input channels on the capture device that're
     * available to the interface.
     * 
     * The value returned is an integer in the range [1,n] such that if the
     * capture device has, for instance, 16 input channels and the interface
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
     * Returns a string that identifies the interface; e.g. "RGBEasy".
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
     * be equal; any scaling of captured frames is expected to be done by VCS and
     * not the capture device.
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
     * Returns the minimum capture resolution supported by the interface.
     * 
     * @note
     * This resolution may be larger - but not smaller - than the minimum
     * capture resolution supported by the capture device.
     * 
     * @see
     * get_maximum_resolution(), get_resolution(), set_resolution()
     */
    virtual resolution_s get_minimum_resolution(void) const = 0;

    /*!
     * Returns the maximum capture resolution supported by the interface.
     * 
     * @note
     * This resolution may be smaller - but not larger - than the maximum
     * capture resolution supported by the capture device.
     * 
     * @see
     * get_minimum_resolution(), get_resolution(), set_resolution()
     */
    virtual resolution_s get_maximum_resolution(void) const = 0;

    /*!
     * Returns number of frames the interface has received from the capture
     * device which VCS was too busy to process and display. These are, in effect,
     * dropped frames.
     * 
     * If this value is above 0, it indicates that VCS is failing to process
     * and display captured frames as fast as the capture device is producing
     * them. This could be a symptom of e.g. an inadequately performant host
     * CPU.
     * 
     * @note
     * This value must be cumulative over the lifetime of the program's
     * execution and must not decrease during that time.
     */
    virtual unsigned get_missed_frames_count(void) const = 0;

    /*!
     * Returns the index value of the capture device's input channel on which the
     * device is currently listening for signals. The value is in the range [0,n-1],
     * where n = get_device_maximum_input_count().
     * 
     * If the capture device has more input channels than are supported by the
     * interface, the interface is expected to map the index to a consecutive range.
     * For example, if channels #1, #5, and #6 on the capture device are available
     * to the interface, capturing on channel #5 would correspond to an index value
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
     * Returns the color depth, in bits, that the interface currently expects
     * the capture device to send captured frames in.
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
     * Returns true if the current capture signal is invalid; false otherwise.
     * 
     * @see
     * has_no_signal()
     */
    virtual bool has_invalid_signal(void) const = 0;

    /*!
     * Returns true if the current capture signal is valid; false otherwise.
     * 
     * @see
     * has_invalid_signal(), has_no_signal()
     */
    virtual bool has_valid_signal(void) const { return !this->has_invalid_signal(); }

    /*!
     * Returns true if the current capture device is invalid; false otherwise.
     * This could happen e.g. if the user (on Linux) opens a /dev/videoX that
     * doesn't provide a compatible capture device.
     */
    virtual bool has_invalid_device(void) const = 0;

    /*!
     * Returns true if the current capture device is valid; false otherwise.
     */
    virtual bool has_valid_device(void) const { return !this->has_invalid_device(); }

    /*!
     * Returns true if there's currently no capture signal on the capture
     * device's active input channel; false otherwise.
     * 
     * @see
     * has_signal(), get_input_channel_idx(), set_input_channel()
     */
    virtual bool has_no_signal(void) const = 0;

    /*!
     * Returns true if the capture device's active input channel is currently
     * receiving a signal; false otherwise.
     *
     * @see
     * has_no_signal(), get_input_channel_idx(), set_input_channel()
     */
    virtual bool has_signal(void) const { return !this->has_no_signal(); }

    /*!
     * Returns true if the device is currently capturing; false otherwise.
     */
    virtual bool is_capturing(void) const = 0;

    /*!
     * Returns the most recently-captured frame's data.
     * 
     * @warning
     * The caller should lock @ref captureMutex while accessing the returned data.
     * Failure to do so may result in a race condition, as the capture interface
     * (and/or the capture device) is permitted to begin operating on these data
     * from outside of the caller's thread when the mutex is unlocked.
     */
    virtual const captured_frame_s &get_frame_buffer(void) const = 0;

    /*************
     * Setters: */

    /*!
     * Called by VCS to notify the interface that VCS has finished processing the
     * latest frame obtained via get_frame_buffer(). The inteface is then free
     * to e.g. overwrite the frame's data.
     * 
     * Returns true on success; false otherwise.
     */
    virtual bool mark_frame_buffer_as_processed(void) { return false; }

    /*!
     * Returns the latest capture event and removes it from the interface's
     * event queue. The caller can then respond to the event; e.g. by calling
     * get_frame_buffer() if the event is a new frame.
     */
    virtual capture_event_e pop_capture_event_queue(void) { return capture_event_e::none; }

    /*!
     * Assigns to the capture device the given video signal parameters.
     * 
     * Returns true on success; false otherwise.
     * 
     * @see
     * get_video_signal_parameters(), get_minimum_video_signal_parameters(),
     * get_maximum_video_signal_parameters(), get_default_video_signal_parameters()
     */
    virtual bool set_video_signal_parameters(const video_signal_parameters_s &p) { (void)p; return true; }

    /*!
     * Sets the capture device's deinterlacing mode.
     * 
     * Returns true on success; false otherwise.
     */
    virtual bool set_deinterlacing_mode(const capture_deinterlacing_mode_e mode) { (void)mode; return false; }

    /*!
     * Tells the capture device to start listening for signals on the given
     * input channel.
     * 
     * Returns true on success; false otherwise.
     * 
     * @see
     * get_input_channel_idx(), get_device_maximum_input_count()
     */
    virtual bool set_input_channel(const unsigned idx) { (void)idx; return false; }

    /*!
     * Tells the capture device to store its captured frames using the given
     * pixel format.
     * 
     * Returns true on success; false otherwise.
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
     * than scaling the frame to 800 x 600 after capturing.
     * 
     * @warning
     * Since this will be set as the capture device's input resolution,
     * captured frames may exhibit artefacting if the resolution doesn't match
     * the video signal's true resolution.
     * 
     * Returns true on success; false otherwise.
     * 
     * @see
     * get_resolution(), get_minimum_resolution(), get_maximum_resolution()
     */
    virtual bool set_resolution(const resolution_s &r) { (void)r; return false; }

    /*!
     * A mutex used to coordinate access to captured data originating via the
     * capture interface (e.g. the frame pixel buffer returned by get_frame_buffer())
     * between the threads of the capture interface, the capture device, and VCS.
     * 
     * The VCS thread will lock this mutex while accessing shared capture data,
     * during which time neither the capture interface nor the capture device
     * should modify those data.
     * 
     * The capture interface should lock this mutex while either it or the capture
     * device is modifying said data.
     */
    std::mutex captureMutex;
};

#endif
