/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * Provides a public interface for interacting with VCS's capture subsystem.
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
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/memory/memory.h"
#include "common/types.h"

struct capture_device_s;

/*!
 * Returns a reference to the currently-active capture device interface. This
 * interface allows the caller to communicate with the capture device.
 * 
 * @note
 * Calling this function will trigger an assertion failure if the device interface
 * isn't available (e.g. because kc_initialize_capture() hasn't been called).
 * 
 * @warning
 * Calling this function before having called kc_initialize_capture() will trigger
 * an assertion failure.
 * 
 * @see
 * kc_initialize_capture(), kc_release_capture()
 */
capture_device_s& kc_capture_device(void);

/*!
 * Initializes the capture subsystem, including the capture hardware.

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
 * Releases the capture subsystem, including the capture hardware.
 * 
 * By default, VCS will call this function on program exit.
 * 
 * Returns true on success; false otherwise.
 * 
 * @warning
 * Calling this function - regardless of its return value - will invalidate
 * the capture device interface reference returned by kc_capture_device().
 * 
 * @see
 * kc_initialize_capture()
 */
void kc_release_capture(void);

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
 * The capture device being used may support only some or none of these modes,
 * and/or might apply them only when receiving an interlaced signal.
 */
enum class capture_deinterlacing_mode_e
{
    weave,
    bob,
    field_0,
    field_1,
};

/*!
 * Enumerates the pixel color formats in which the capture subsystem may provide
 * frame data.
 */
enum class capture_pixel_format_e
{
    //! 16 bits per pixel: 5 bits for red, green and blue, and 1 bit of padding.
    rgb_555,

    //! 16 bits per pixel: 5 bits for red and blue, 6 bits for green.
    rgb_565,

    //! 32 bits per pixel: 8 bits for red, green and blue, and 8 bits of padding.
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

    //! Total enumerator count; should remain the last item in the list.
    num_enumerators
};

/*!
 * Stores the data of an image (captured frame) received from a capture device.
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

#endif
