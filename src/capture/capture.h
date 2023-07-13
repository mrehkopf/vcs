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
 *   1. Call kc_initialize_capture() to initialize the subsystem. Note that the
 *      function should be called only once per program execution.
 *
 *   2. Use the interface functions to interact with the subsystem. For instance,
 *      kc_frame_buffer() returns the most recent captured frame's data.
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
#include "common/propagate/vcs_event.h"
#include "main.h"

struct video_mode_s;

/*!
 * Fired when the capture subsystem makes a new captured frame available. A
 * reference to the frame's data is provided as an argument to listeners; a
 * listener can assume that the data will remain valid until the listener returns.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time of capture, but rather once VCS has polled the capture subsystem (via
 * kc_pop_event_queue()) to find that there is a new frame.
 * 
 * 
 * 
 * @code
 * // Register an event listener that gets run each time a new frame is captured.
 * kc_ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     // The frame's data is available to this listener until the function
 *     // returns. If we wanted to keep hold of the data for longer, we'd need
 *     // to make a copy of it.
 * });
 * @endcode
 * 
 * @code
 * // Feed captured frames into the scaler subsystem.
 * kc_ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     printf("Captured in %lu x %lu.\n", frame.r.w, frame.r.h);
 *     ks_scale_frame(frame);
 * });
 * 
 * // Receive a notification whenever a frame has been scaled.
 * ks_ev_new_scaled_image.listen([](const image_s &image)
 * {
 *     printf("Scaled to %lu x %lu.\n", image.resolution.w, image.resolution.h);
 * });
 * @endcode
 * 
 * @note
 * The capture mutex must be locked before firing this event, including before
 * acquiring the frame reference from kc_frame_buffer().
 * 
 * @see
 * kc_frame_buffer(), kc_mutex()
 */
extern vcs_event_c<const captured_frame_s&> kc_ev_new_captured_frame;

/*!
 * Fired when the capture device reports that the video mode of the currently-captured
 signal has changed.
 * 
 * This event is to be treated as a proposal from the capture device that the given
 * video mode now best fits the captured signal. You can accept the proposal by firing
 * the @ref kc_ev_new_video_mode event, call kc_set_resolution() to force the
 * capture device to use a different mode, or do nothing to retain the existing mode.
 * 
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the video mode changes, but rather once VCS has polled the capture subsystem
 * (via kc_pop_event_queue()) to find that the mode has changed.
 * 
 * @code
 * // A sample implementation that approves the proposed video mode if there's
 * // no alias for it, and otherwise forces the alias mode.
 * kc_ev_new_proposed_video_mode.listen([](const video_mode_s &videoMode)
 * {
 *     if (ka_has_alias(videoMode.resolution))
 *     {
 *         kc_set_resolution(ka_aliased(videoMode.resolution));
 *     }
 *     else
 *     {
 *         kc_ev_new_video_mode.fire(videoMode);
 *     }
 * });
 * @endcode
 * 
 * @see
 * kc_ev_new_video_mode, kc_set_resolution()
 */
extern vcs_event_c<const video_mode_s&> kc_ev_new_proposed_video_mode;

/*!
 * Fired when the capture subsystem has begun capturing in a new video mode.
 * 
 * It's not guaranteed that the new video mode is different from the previous
 * one, although usually it will be.
 * 
 * @see
 * kc_ev_new_proposed_video_mode, kc_set_resolution()
 */
extern vcs_event_c<const video_mode_s&> kc_ev_new_video_mode;

/*!
 * Fired when the capture device is switched to a different input channel.
 */
extern vcs_event_c<unsigned> kc_ev_input_channel_changed;

/*!
 * Fired when the capture subsystem reports its capture device to be invalid in a
 * way that renders the device unusable to the subsystem.
 * 
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the device is detected to be invalid, but rather once VCS has polled the capture
 * subsystem (via kc_pop_event_queue()) to find that such is the case.
 */
extern vcs_event_c<void> kc_ev_invalid_device;

/*!
 * Fired when the capture device loses its input signal. This event implies that
 * the device was previously receiving a signal.
 * 
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the signal is lost, but rather once VCS has polled the capture subsystem (via
 * kc_pop_event_queue()) to find that such is the case.
 * 
 * @code
 * // Print a message every time the capture signal is lost.
 * kc_ev_signal_lost.listen([]
 * {
 *     printf("The signal was lost.\n");
 * });
 * @endcode
 * 
 * @see
 * kc_ev_signal_gained
 */
extern vcs_event_c<void> kc_ev_signal_lost;

/*!
 * Fired when the capture device begins receiving an input signal. This event
 * implies that the device was previously not receiving a signal.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the signal is gained, but rather once VCS has polled the capture subsystem (via
 * kc_pop_event_queue()) to find that such is the case.
 * 
 * @see
 * kc_ev_signal_lost
 */
extern vcs_event_c<void> kc_ev_signal_gained;

/*!
 * Fired when the capture device reports its input signal to be invalid (e.g.
 * out of range).
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time the invalid signal began to be received, but rather once VCS has polled
 * the capture subsystem (via kc_pop_event_queue()) to find that such a
 * condition exists.
 */
extern vcs_event_c<void> kc_ev_invalid_signal;

/*!
 * Fired when an error occurs in the capture subsystem from which the
 * subsystem can't recover.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time the error occurs, but rather once VCS has polled the capture subsystem
 * (via kc_pop_event_queue()) to find that there has been such an error.
 */
extern vcs_event_c<void> kc_ev_unrecoverable_error;

/*!
 * VCS will periodically query the capture subsystem for the latest capture events.
 * This enumerates the range of capture events that the capture subsystem can report
 * back.
 * 
 * @see
 * kc_pop_event_queue()
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
    resolution_s r;
    uint8_t *pixels;
};

struct video_signal_parameters_s
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

    static video_signal_parameters_s from_capture_device(const std::string &nameSpace = "");
    static void to_capture_device(const video_signal_parameters_s &params, const std::string &nameSpace = "");
};

/*!
 * Returns a reference to a mutex that should be locked by the capture subsystem
 * while it's accessing data shared with the rest of VCS (e.g. capture event flags
 * or the capture frame buffer), and which the rest of VCS should lock while
 * accessing that data.
 *
 * Failure to observe this mutex when accessing capture subsystem data may result
 * in a race condition, as the capture subsystem is allowed to run outside of the
 * main VCS thread.
 *
 * @code
 * // Code running in the main VCS thread.
 * 
 * // Blocks execution until the capture mutex allows us to access the capture data.
 * std::lock_guard<std::mutex> lock(kc_mutex());
 *
 * // Handle the most recent capture event (having locked the mutex prevents
 * // the capture subsystem from pushing new events while we're doing this).
 * switch (kc_pop_event_queue())
 * {
 *     // ...
 * }
 * @endcode
 *
 * @note
 * If the capture subsystem finds the capture mutex locked when the capture device
 * sends in a new frame, the frame will be discarded rather than waiting for the
 * lock to be released.
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
 * @code
 * kc_initialize_capture();
 * 
 * // This listener function gets called each time a frame is captured.
 * kc_ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     printf("Captured a frame (%lu x %lu)\n", frame.r.w, frame.r.h);
 * });
 * @endcode
 *
 * @see
 * kc_ev_new_captured_frame
 */
subsystem_releaser_t kc_initialize_capture(void);

/*!
 * Initializes the capture device.
 *
 * Throws on failure.
 *
 * @warning
 * Don't call this function directly. Instead, call kc_initialize_capture(),
 * which will initialize both the capture device and the capture subsystem.
 *
 * @see
 * kc_initialize_capture(), kc_release_device()
 */
void kc_initialize_device(void);

/*!
 * Releases the capture device.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * Don't call this function directly. The capture subsystem will call it as
 * necessary.
 *
 * @see
 * kc_initialize_capture(), kc_initialize_device()
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
 * kc_ev_signal_gained, kc_ev_signal_lost
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
 * kc_mutex(), kc_ev_new_captured_frame
 */
const captured_frame_s& kc_frame_buffer(void);

/*!
 * Processes the most recent or most important capture event and removes it from
 * the capture subsystem's event queue.
 */
capture_event_e kc_pop_event_queue(void);

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
 * @see
 * kc_set_device_property()
 */
double kc_device_property(const std::string key);

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
 * Depending on the implementation of the capture device in VCS, modifying a
 * property may mutate the device state. For example, changing an
 * "input channel index" property may cause the capture device to switch to a
 * different input channel.
 *
 * @see
 * kc_device_property()
 */
bool kc_set_device_property(const std::string key, double value);

#endif
