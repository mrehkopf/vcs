/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The capture subsystem interface.
 *
 * The capture subsystem is responsible for mediating exchange between VCS and
 * the capture device; including initializing the device and providing VCS with
 * access to the frame buffer(s) associated with the device.
 * 
 * The capture subsystem is an exceptional subsystem in that it's allowed to run
 * outside of the main VCS thread. This freedom -- in what is otherwise a
 * single-threaded application -- is granted because some capture devices require
 * out-of-thread callbacks or the like.
 * 
 * The multithreaded capture subsystem will typically have an idle monitoring
 * thread that waits for the capture device to send in data. When data comes in,
 * the thread will copy it to a local memory buffer to be operated on in the
 * main VCS thread when it's ready to do so.
 * 
 * Because of the out-of-thread nature of this subsystem, it's important that
 * accesses to its memory buffers are synchronized using the capture mutex (see
 * kc_mutex()).
 *
 * ## Usage
 *
 *   1. Call kc_initialize_capture() to initialize the capture subsystem.
 *
 *   2. Use the interface functions to interact with the subsystem, making sure
 *      to observe the capture mutex (kc_mutex()). For example, kc_frame_buffer()
 *      returns the most recent captured frame's data.
 *
 *   3. VCS will automatically release the subsystem on program termination.
 * 
 * ## Implementing support for new capture devices
 * 
 * (This section is pending a rewrite.)
 */

#ifndef VCS_CAPTURE_CAPTURE_H
#define VCS_CAPTURE_CAPTURE_H

#include <unordered_map>
#include <vector>
#include <mutex>
#include <any>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/refresh_rate.h"
#include "common/types.h"
#include "common/vcs_event/vcs_event.h"
#include "main.h"

struct video_mode_s;

/*!
 * VCS will periodically query the capture subsystem for the latest capture events.
 * This enumerates the range of capture events that the capture subsystem can report
 * back.
 * 
 * @see
 * kc_process_next_capture_event()
 */
enum class capture_event_e
{
    /*!
     * No capture events to report. Although some capture events may have occurred,
     * the capture subsystem chooses to not inform VCS of them.
     */
    none,

    /*!
     * Same as 'none', but the capture subsystem also thinks VCS shouldn't send
     * a new query for capture events for some short while (milliseconds); e.g.
     * because the capture device is currently not receiving a signal and so isn't
     * expected to produce events in the immediate future.
     */
    sleep,

    /*! The capture device has just lost its input signal.*/
    signal_lost,

    /*! The capture device has just gained an input signal.*/
    signal_gained,

    /*!
     * The capture device has sent in a new frame, whose data can be queried via
     * get_frame_buffer().
     */
    new_frame,

    /*! The capture device's input signal has changed in resolution or refresh rate.*/
    new_video_mode,

    /*! The capture device's current input signal is invalid (e.g. out of range).*/
    invalid_signal,

    /*! The capture device isn't available for use.*/
    invalid_device,

    /*!
     * An error has occurred with the capture device from which the capture
     * subsystem can't recover.
     */
    unrecoverable_error,

    /*! Total enumerator count. Should remain the last item in the list.*/
    num_enumerators
};

struct video_mode_s
{
    resolution_s resolution;
    refresh_rate_s refreshRate;
};

enum class signal_format_e
{
    none = 0,
    analog,
    digital,
};

/*!
 * @brief
 * Stores the data of an image received from a capture device.
 */
struct captured_frame_s
{
    resolution_s resolution;
    uint8_t *pixels;
};

struct video_signal_properties_s
{
    resolution_s r; // For legacy (VCS <= 1.6.5) support.

    long brightness;
    long contrast;
    long redBrightness;
    long redContrast;
    long greenBrightness;
    long greenContrast;
    long blueBrightness;
    long blueContrast;
    unsigned long horizontalSize;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;

    static video_signal_properties_s from_capture_device_properties(const std::string &nameSpace = "");
    static void to_capture_device_properties(const video_signal_properties_s &params, const std::string &nameSpace = "");
};

/*!
 * Returns a reference to a mutex which a thread should acquire before invoking the
 * functions of the capture subsystem interface, and which the capture subsystem
 * should acquire before mutating data to which the interface provides access.
 *
 * @code
 * // Code running in the main VCS thread.
 * 
 * // We haven't acquired the capture mutex, so the pixel data in the subsystem's
 * // frame buffer is liable to be overwritten at any time by a capture thread.
 * const auto &frameBuffer = kc_frame_buffer();
 * 
 * {
 *     // Blocks execution until the capture subsystem is ready to share data.
 *     std::lock_guard<std::mutex> lock(kc_mutex());
 * 
 *     // We can now access the pixel data without fear of it being overwritten.
 *     unsigned red = frameBuffer.pixels[0];
 * 
 *     // The listeners of this event are also in the scope of the mutex when the
 *     // event is fired here.
 *     ev_new_captured_frame.fire(frameBuffer);
 * }
 * 
 * // The mutex is now unlocked by going out of scope, so the data in the frame
 * // buffer is again liable to be overwritten at any time.
 * @endcode
 * 
 * In effect, it can be assumed that the caller of the interface is the main VCS
 * thread, which also runs the capture subsystem, so the subsystem generally only
 * needs to acquire the mutex when mutating shared data in a thread it has spawned
 * for itself.
 * 
 * @code
 * // Code running in a capture thread spawned by the capture subsystem.
 * 
 * if (deviceHasCapturedNewFrame)
 * {
 *     // Wait until VCS is no longer accessing the subsystem's frame buffer.
 *     std::lock_guard<std::mutex> lock(kc_mutex());
 * 
 *     // Copy the frame's data from device memory into the subsystem's frame buffer.
 *     memcpy(frameBuffer.pixels, ...);
 * }
 * @endcode
 *
 */
std::mutex& kc_mutex(void);

/*!
 * Initializes the capture subsystem.
 *
 * By default, VCS will call this function automatically on program startup.
 *
 * Returns a function that releases the capture subsystem.
 * 
 * @note
 * Will trigger an assertion failure if the initialization fails.
 * 
 * @warning
 * This function should be called only once per program execution.
 * 
 * @code
 * kc_initialize_capture();
 * 
 * // This listener function gets called each time a frame is captured.
 * ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     printf("Captured a frame (%lu x %lu)\n", frame.r.w, frame.r.h);
 * });
 * @endcode
 */
subsystem_releaser_t kc_initialize_capture(void);

/*!
 * Prepares the capture device for use, including starting the capture stream.
 *
 * Throws on failure.
 *
 * @warning
 * You don't ever need to call this function manually; VCS will call it automatically
 * when appropriate.
 *
 * @see
 * kc_initialize_capture(), kc_release_device()
 */
void kc_initialize_device(void);

/*!
 * Releases the capture device, including stopping the capture stream and freeing
 * any on-device buffers that were in use for the capture subsystem.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * You don't ever need to call this function manually; VCS will call it
 * automatically when appropriate.
 */
bool kc_release_device(void);

/*!
 * Returns the count of frames the capture subsystem has received from the
 * capture device that VCS was too busy to process and display; In other words,
 * a count of dropped frames.
 *
 * If this value goes above 0, it indicates that VCS is failing to process and
 * display captured frames as fast as the capture device is producing them.
 * 
 * @note
 * The value is monotonically cumulative over the lifetime of the program's
 * execution, guaranteed not to decrease during that time.
 * 
 * @see
 * kc_mutex()
 */
unsigned kc_dropped_frames_count(void);

/*!
 * Returns true if the capture device's active input channel is currently
 * receiving a signal; false otherwise.
 *
 * @note
 * Will return false also when the capture device is receiving a signal that is
 * out of range.
 *
 * @see
 * kc_mutex(), ev_capture_signal_gained, ev_capture_signal_lost
 */
bool kc_has_signal(void);

/*!
 * Returns a reference to the most recent captured frame.
 * 
 * To ensure that the frame buffer's data isn't modified by another thread while
 * you're accessing it, lock the capture mutex before calling this function and
 * release it when you're done.
 *
 * @code
 * {
 *     // The capture mutex should be locked first, to ensure the frame buffer
 *     // isn't modified by another thread while we're accessing its data.
 *     std::lock_guard<std::mutex> lock(kc_mutex());
 *
 *     const auto &frameBuffer = kc_frame_buffer();
 *    // Access the frame buffer's data...
 * }
 * @endcode
 *
 * @see
 * kc_mutex(), ev_new_captured_frame
 */
const captured_frame_s& kc_frame_buffer(void);

/*!
 * Asks the capture subsystem to process the most recent or most important capture
 * event in its queue of events received from the capture device; removing the event
 * from the queue before returning.
 * 
 * In processing the event, the function may call other subsystems of VCS (directly
 * or by firing [events](@ref src/common/vcs_event/vcs_event.h)) to process
 * relevant data; e.g. the scaler and display subsystems if the event is a new
 * captured frame.
 * 
 * @see
 * kc_mutex()
 */
capture_event_e kc_process_next_capture_event(void);

/*!
 * Returns the value of the capture device property identified by @p key.
 *
 * @code
 * const auto inputResolution = resolution_s{
 *     .w = unsigned(kc_device_property("width")),
 *     .h = unsigned(kc_device_property("height"))
 * };
 * @endcode
 *
 * Capture device properties are key--value pairs that provide metadata about the
 * capture device; for example, its current capture resolution. They also act as
 * a control surface, as they're capable of mutating the capture state (see
 * kc_set_device_property()).
 *
 * The specific meaning of a key--value pair depends ultimately on the
 * implementation of the device's interface in VCS, although there are certain
 * standard keys that all implementations are expected to support. (A full list
 * of standard keys will be added to this documentation.)
 *
 * @code
 * // The static resolution_s::from_capture_device() function calls
 * // kc_device_property("width") and kc_device_property("height")
 * // to return the current capture resolution.
 * const auto inputResolution = resolution_s::from_capture_device();
 * @endcode
 * 
 * @warning
 * The device property framework is intended for use within the main VCS thread
 * only; as such, this function should only be called from the main VCS thread.
 * The capture mutex (kc_mutex()) doesn't need to be acquired prior to calling
 * the function.
 *
 * @see
 * kc_set_device_property()
 */
double kc_device_property(const std::string &key);

/*!
 * Assigns @p value to the capture device property identified by @p key.
 *
 * Returns true if the value was assigned successfully; false otherwise.
 *
 * @code
 * if (kc_set_device_property("width: minimum", MIN_CAPTURE_WIDTH))
 * {
 *     // kc_device_property("width: minimum") == MIN_CAPTURE_WIDTH
 * }
 * @endcode
 *
 * Depending on the implementation of the capture subsystem, modifying a property
 * may lead to a corresponding mutation in device state. For example, changing aa
 * "channel" property may cause the capture device to switch to a different input
 * channel.
 * 
 * @warning
 * The device property framework is intended for use within the main VCS thread
 * only; as such, this function should only be called from the main VCS thread.
 * The capture mutex (kc_mutex()) doesn't need to be acquired prior to calling
 * the function.
 *
 * @see
 * kc_device_property()
 */
bool kc_set_device_property(const std::string &key, double value);

#endif
