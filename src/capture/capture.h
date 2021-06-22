/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The interface to VCS's capture subsystem.
 *
 * The capture subsystem is responsible for mediating exchanges between VCS and
 * the capture device; including initializing the capture device and providing
 * VCS with access to the frame buffer(s) associated with the device.
 *
 * Usage:
 *
 *   1. Call kc_initialize_capture() to initialize the subsystem. This is VCS's
 *      default startup behavior.
 *
 *   2. Use the interface returned by kc_capture_device() to interact with the
 *      capture device.
 *
 *   3. Call kc_release_capture() to release the subsystem. This is VCS's default
 *      exit behavior.
 *
 */

#ifndef VCS_CAPTURE_CAPTURE_H
#define VCS_CAPTURE_CAPTURE_H

#include <vector>
#include <mutex>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/refresh_rate.h"
#include "common/memory/memory.h"
#include "common/types.h"

struct capture_device_s;

std::mutex& kc_capture_mutex(void);

/*!
 * Initializes the capture subsystem.
 *
 * By default, VCS will call this function on program startup.
 *
 * @note
 * Will trigger an assertion failure if the initialization fails.
 *
 * @see
 * kc_release_capture()
 */
void kc_initialize_capture(void);

/*!
 * Initializes the capture device.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * Don't call this function directly. Instead, call kc_initialize_capture(),
 * which will initialize both the capture device and the capture subsystem
 * (they are interlinked).
 *
 * @see
 * kc_initialize_capture(), kc_release_device()
 */
bool kc_initialize_device(void);

/*!
 * Releases the capture subsystem.
 *
 * By default, VCS will call this function on program exit.
 *
 * @see
 * kc_initialize_capture()
 */
void kc_release_capture(void);

/*!
 * Releases the capture device.
 *
 * By default, this function will be called by kc_release_capture(), which
 * you should call if you want to release the capture device.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * Don't call this function directly. Instead, call kc_release_capture(), which
 * will release both the capture device and the capture subsystem (they are
 * interlinked).
 *
 * @see
 * kc_initialize_capture(), kc_initialize_device()
 */
bool kc_release_device(void);

/*!
 * Asks the capture device to set its input resolution to the one given,
 * overriding the current input resolution.
 *
 * @note
 * If the resolution of the captured signal doesn't match this resolution, the
 * captured image may become corrupted until the proper resolution is set.
 *
 * @warning
 * This function may become obsolete in a future version of VCS, in which case
 * its functionality will likely be merged into the capture device interface
 * (see kc_capture_device()).
 */
bool kc_force_input_resolution(const resolution_s &r);

/*!
 * Enumerates the de-interlacing modes recognized by the capture subsystem.
 *
 * @note
 * The capture subsystem itself doesn't apply de-interlacing, it just asks the
 * capture device to do so. The capture device in turn may support only some or
 * none of these modes, and/or might apply them only when receiving an
 * interlaced signal.
 */
enum class capture_deinterlacing_mode_e
{
    weave,
    bob,
    field_0,
    field_1,
};

/*!
 * Enumerates the color formats recognized by the capture subsystem. Frames output
 * by the subsystem will have their pixel data in one of these formats.
 *
 * The capture device may support some, none, or all of these formats. Any
 * capability queries should be made via the capture device interface.
 *
 * @see
 * captured_frame_s, capture_device_s
 */
enum class capture_pixel_format_e
{
    //! 16 bits per pixel: 5 bits for red, 5 bits for green, 5 bits for blue, and
    //! 1 bit of padding. No alpha.
    rgb_555,

    //! 16 bits per pixel: 5 bits for red, 6 bits for green, and 5 bits for blue.
    //! No alpha.
    rgb_565,

    //! 32 bits per pixel: 8 bits for red, 8 bits for green, 8 bits for blue, and
    //! 8 bits of padding. No alpha.
    rgb_888,
};

/*!
 * VCS will periodically query the capture subsystem for the latest capture events.
 * This enumerates the range of capture events that the capture subsystem can report
 * back.
 */
enum class capture_event_e
{
    //! No capture events to report. Although some capture events may have occurred,
    //! the capture subsystem chooses to not inform VCS of them.
    none,

    //! Same as 'none', but the capture subsystem also thinks VCS shouldn't send
    //! a new query for capture events for some short while (milliseconds); e.g.
    //! because the capture device is currently not receiving a signal and so isn't
    //! expected to produce events in the immediate future.
    sleep,

    //! The capture device has just lost its input signal.
    signal_lost,

    //! The capture device has sent a new frame, whose data is now available tp
    //! VCS via the capture device interface.
    new_frame,

    //! The capture device's input signal has changed in resolution or refresh rate.
    //! The new mode parameters can be queried via the capture device interface.
    new_video_mode,

    //! The capture device's current input signal is invalid (e.g. out of range).
    invalid_signal,

    //! The capture device isn't available for use.
    invalid_device,

    //! An error has occurred within the capture subsystem from which the subsystem
    //! can't recover.
    unrecoverable_error,

    //! Total enumerator count. Should remain the last item in the list.
    num_enumerators
};

/*!
 * @brief
 * Stores the data of an image received from a capture device.
 */
struct captured_frame_s
{
    resolution_s r;

    capture_pixel_format_e pixelFormat;

    heap_bytes_s<u8> pixels;

    // Will be set to true after the frame's data has been processed for
    // display and is no longer needed.
    bool processed = false;
};

struct signal_info_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
};

struct video_signal_parameters_s
{
    resolution_s r; // For legacy (VCS <= 1.6.5) support.

    long overallBrightness;
    long overallContrast;

    long redBrightness;
    long redContrast;

    long greenBrightness;
    long greenContrast;

    long blueBrightness;
    long blueContrast;

    unsigned long horizontalScale;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;
};







/*!
 * Returns true if the capture device is capable of capturing from a
 * component video source; false otherwise.
 */
bool kc_device_supports_component_capture(void);

/*!
 * Returns true if the capture device is capable of capturing from a
 * composite video source; false otherwise.
 */
bool kc_device_supports_composite_capture(void);

/*!
 * Returns true if the capture device supports hardware de-interlacing;
 * false otherwise.
 */
bool kc_device_supports_deinterlacing(void);

/*!
 * Returns true if the capture device is capable of capturing from an
 * S-Video source; false otherwise.
 */
bool kc_device_supports_svideo(void);

/*!
 * Returns true if the capture device is capable of streaming frames via
 * direct memory access (DMA); false otherwise.
 */
bool kc_device_supports_dma(void);

/*!
 * Returns true if the capture device is capable of capturing from a
 * digital (DVI) source; false otherwise.
 */
bool kc_device_supports_dvi(void);

/*!
 * Returns true if the capture device is capable of capturing from an
 * analog (VGA) source; false otherwise.
 */
bool kc_device_supports_vga(void);

/*!
 * Returns true if the capture device is capable of capturing in YUV
 * color; false otherwise.
 */
bool kc_device_supports_yuv(void);

/*!
 * Returns the number of input channels on the capture device that're
 * available to the interface.
 *
 * The value returned is an integer in the range [1,n] such that if the
 * capture device has, for instance, 16 input channels and the interface
 * can use two of them, 2 is returned.
 *
 * @see
 * kc_get_input_channel_idx()
 */
int kc_get_device_maximum_input_count(void);

/*!
 * Returns a string that identifies the capture device's firmware version;
 * e.g. "14.12.3".
 *
 * Will return "Unknown" if the firmware version is not known.
 *
 * @see
 * kc_get_device_driver_version()
 */
std::string kc_get_device_firmware_version(void);

/*!
 * Returns a string that identifies the capture device's driver version;
 * e.g. "14.12.3".
 *
 * Will return "Unknown" if the firmware version is not known.
 *
 * @see
 * kc_get_device_firmware_version()
 */
std::string kc_get_device_driver_version(void);

/*!
 * Returns a string that identifies the capture device; e.g. "Datapath
 * VisionRGB-PRO2".
 *
 * Will return "Unknown" if no name is available.
 */
std::string kc_get_device_name(void);

/*!
 * Returns a string that identifies the interface; e.g. "RGBEasy".
 */
std::string kc_get_api_name(void);

/*!
 * Returns the capture device's current video signal parameters.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_minimum_video_signal_parameters(),
 * kc_get_maximum_video_signal_parameters(), kc_get_default_video_signal_parameters()
 */
video_signal_parameters_s kc_get_video_signal_parameters(void);

/*!
 * Returns the capture device's default video signal parameters.
 *
 * @see
 * kc_get_video_signal_parameters(), kc_get_video_signal_parameters(),
 * kc_get_minimum_video_signal_parameters(), kc_get_maximum_video_signal_parameters()
 */
video_signal_parameters_s kc_get_default_video_signal_parameters(void);

/*!
 * Returns the minimum value supported by the capture device for each video
 * signal parameter.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_video_signal_parameters(),
 * kc_get_maximum_video_signal_parameters(), kc_get_default_video_signal_parameters()
 */
video_signal_parameters_s kc_get_minimum_video_signal_parameters(void);

/*!
 * Returns the maximum value supported by the capture device for each video
 * signal parameter.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_video_signal_parameters(),
 * kc_get_minimum_video_signal_parameters(), kc_get_default_video_signal_parameters()
 */
video_signal_parameters_s kc_get_maximum_video_signal_parameters(void);

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
 * (returned from kc_get_frame_buffer()), as the capture device's resolution
 * may have changed since that frame was captured.
 *
 * @see
 * kc_set_resolution(), kc_get_minimum_resolution(), kc_get_maximum_resolution()
 */
resolution_s kc_get_resolution(void);

/*!
 * Returns the minimum capture resolution supported by the interface.
 *
 * @note
 * This resolution may be larger - but not smaller - than the minimum
 * capture resolution supported by the capture device.
 *
 * @see
 * kc_get_maximum_resolution(), kc_get_resolution(), kc_set_resolution()
 */
resolution_s kc_get_minimum_resolution(void);

/*!
 * Returns the maximum capture resolution supported by the interface.
 *
 * @note
 * This resolution may be smaller - but not larger - than the maximum
 * capture resolution supported by the capture device.
 *
 * @see
 * kc_get_minimum_resolution(), kc_get_resolution(), kc_set_resolution()
 */
resolution_s kc_get_maximum_resolution(void);

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
unsigned kc_get_missed_frames_count(void);

/*!
 * Returns the index value of the capture device's input channel on which the
 * device is currently listening for signals. The value is in the range [0,n-1],
 * where n = kc_get_device_maximum_input_count().
 *
 * If the capture device has more input channels than are supported by the
 * interface, the interface is expected to map the index to a consecutive range.
 * For example, if channels #1, #5, and #6 on the capture device are available
 * to the interface, capturing on channel #5 would correspond to an index value
 * of 1, with index 0 being channel #1 and index 2 channel #6.
 *
 * @see
 * kc_get_device_maximum_input_count()
 */
unsigned kc_get_input_channel_idx(void);

/*!
 * Returns the refresh rate of the current capture signal.
 */
refresh_rate_s kc_get_refresh_rate(void);

/*!
 * Returns the color depth, in bits, that the interface currently expects
 * the capture device to send captured frames in.
 *
 * For RGB888 frames the color depth would be 32; 16 for RGB565 frames; etc.
 *
 * The color depth of a given frame as returned from kc_get_frame_buffer() may
 * be different from this value e.g. if the capture color depth was changed
 * just after the frame was captured.
 */
unsigned kc_get_color_depth(void);

/*!
 * Returns the pixel format that the capture device is currently storing
 * its captured frames in.
 *
 * The pixel format of a given frame as returned from kc_get_frame_buffer() may
 * be different from this value e.g. if the capture pixel format was changed
 * just after the frame was captured.
 */
capture_pixel_format_e kc_get_pixel_format(void);

/*!
 * Returns true if the current capture signal is invalid; false otherwise.
 *
 * @see
 * has_no_signal()
 */
bool kc_has_invalid_signal(void);

/*!
 * Returns true if the current capture signal is valid; false otherwise.
 *
 * @see
 * has_invalid_signal(), has_no_signal()
 */
bool kc_has_valid_signal(void);

/*!
 * Returns true if the current capture device is invalid; false otherwise.
 * This could happen e.g. if the user (on Linux) opens a /dev/videoX that
 * doesn't provide a compatible capture device.
 */
bool kc_has_invalid_device(void);

/*!
 * Returns true if the current capture device is valid; false otherwise.
 */
bool kc_has_valid_device(void);

/*!
 * Returns true if there's currently no capture signal on the capture
 * device's active input channel; false otherwise.
 *
 * @see
 * has_signal(), kc_get_input_channel_idx(), kc_set_input_channel()
 */
bool kc_has_no_signal(void);

/*!
 * Returns true if the capture device's active input channel is currently
 * receiving a signal; false otherwise.
 *
 * @see
 * has_no_signal(), kc_get_input_channel_idx(), kc_set_input_channel()
 */
bool kc_has_signal(void);

/*!
 * Returns true if the device is currently capturing; false otherwise.
 */
bool kc_is_capturing(void);

/*!
 * Returns the most recently-captured frame's data.
 *
 * @warning
 * The caller should lock @ref captureMutex while accessing the returned data.
 * Failure to do so may result in a race condition, as the capture interface
 * (and/or the capture device) is permitted to begin operating on these data
 * from outside of the caller's thread when the mutex is unlocked.
 */
const captured_frame_s &kc_get_frame_buffer(void);

/*!
 * Called by VCS to notify the interface that VCS has finished processing the
 * latest frame obtained via kc_get_frame_buffer(). The inteface is then free
 * to e.g. overwrite the frame's data.
 *
 * Returns true on success; false otherwise.
 */
bool kc_mark_frame_buffer_as_processed(void);

/*!
 * Returns the latest capture event and removes it from the interface's
 * event queue. The caller can then respond to the event; e.g. by calling
 * kc_get_frame_buffer() if the event is a new frame.
 */
capture_event_e kc_pop_capture_event_queue(void);

/*!
 * Assigns to the capture device the given video signal parameters.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_video_signal_parameters(), kc_get_minimum_video_signal_parameters(),
 * kc_get_maximum_video_signal_parameters(), kc_get_default_video_signal_parameters()
 */
bool kc_set_video_signal_parameters(const video_signal_parameters_s &p);

/*!
 * Sets the capture device's deinterlacing mode.
 *
 * Returns true on success; false otherwise.
 */
bool kc_set_deinterlacing_mode(const capture_deinterlacing_mode_e mode);

/*!
 * Tells the capture device to start listening for signals on the given
 * input channel.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_input_channel_idx(), kc_get_device_maximum_input_count()
 */
bool kc_set_input_channel(const unsigned idx);

/*!
 * Tells the capture device to store its captured frames using the given
 * pixel format.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_pixel_format()
 */
bool kc_set_pixel_format(const capture_pixel_format_e pf);

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
 * kc_get_resolution(), kc_get_minimum_resolution(), kc_get_maximum_resolution()
 */
bool kc_set_resolution(const resolution_s &r);








#endif
