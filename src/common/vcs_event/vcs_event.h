/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*! @file
 *
 * @brief
 * Application-wide event system.
 *
 * VCS's application-wide event system provides a framework for the various subsystems
 * of the application to receive notification of program events.
 *
 * ## Usage
 *
 * Simply subscribe a listener function to the event you're interested in being
 * notified of:
 *
 * @code
 * ev_frames_per_second.listen([](unsigned fps)
 * {
 *     printf("Capturing at %u FPS\n", fps);
 * });
 *
 * // The listener will execute whenever ev_frames_per_second.fire() is invoked.
 * @endcode
 *
 * If there are multiple listeners to an event, they'll be called synchronously in the
 * order they were subscribed.
 *
 * @note
 * Listener functions can't be unsubscribed.
 */

#ifndef VCS_COMMON_VCS_EVENT_VCS_EVENT_H
#define VCS_COMMON_VCS_EVENT_VCS_EVENT_H

#include <functional>
#include <list>

struct captured_frame_s;
struct video_preset_s;
struct video_mode_s;
struct resolution_s;
struct image_s;

// An event that passes an argument to its event handlers.
template <typename T>
class vcs_event_c
{
public:
    void listen(std::function<void(T)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    // For event handlers that want to ignore the callback argument.
    void listen(std::function<void(void)> handlerFn)
    {
        this->subscribedHandlersNoArgs.push_back(handlerFn);

        return;
    }

    void fire(T value) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn(value);
        }

        for (const auto &handlerFn: this->subscribedHandlersNoArgs)
        {
            handlerFn();
        }

        return;
    }

private:
    std::list<std::function<void(T)>> subscribedHandlers;
    std::list<std::function<void(void)>> subscribedHandlersNoArgs;
};

// An event that passes no arguments to its event handlers.
template <>
class vcs_event_c<void>
{
public:
    void listen(std::function<void(void)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    void fire(void) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn();
        }

        return;
    }

private:
    std::list<std::function<void(void)>> subscribedHandlers;
};

/*!
 * Fired when the capture subsystem makes a new captured frame available. A
 * reference to the frame's data is provided as an argument to listeners; a
 * listener can assume that the data will remain valid until the listener returns.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time of capture, but rather once VCS has polled the capture subsystem (via
 * kc_process_next_capture_event()) to find that there is a new frame.
 *
 * @code
 * // Register an event listener that gets run each time a new frame is captured.
 * ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     // The frame's data is available to this listener until the function
 *     // returns. If we wanted to keep hold of the data for longer, we'd need
 *     // to make a copy of it.
 * });
 * @endcode
 *
 * @code
 * // Feed captured frames into the scaler subsystem.
 * ev_new_captured_frame.listen([](const captured_frame_s &frame)
 * {
 *     printf("Captured in %lu x %lu.\n", frame.r.w, frame.r.h);
 *     ks_scale_frame(frame);
 * });
 *
 * // Receive a notification whenever a frame has been scaled.
 * ev_new_output_image.listen([](const image_s &image)
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
extern vcs_event_c<const captured_frame_s&> ev_new_captured_frame;

/*!
 * Fired when the capture device reports that the video mode of the currently-captured
 signal has changed.
 *
 * This event is to be treated as a proposal from the capture device that the given
 * video mode now best fits the captured signal. You can accept the proposal by firing
 * the @ref ev_new_video_mode event, call kc_set_resolution() to force the
 * capture device to use a different mode, or do nothing to retain the existing mode.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the video mode changes, but rather once VCS has polled the capture subsystem
 * (via kc_process_next_capture_event()) to find that the mode has changed.
 *
 * @code
 * // A sample implementation that approves the proposed video mode if there's
 * // no alias for it, and otherwise forces the alias mode.
 * ev_new_proposed_video_mode.listen([](const video_mode_s &videoMode)
 * {
 *     if (ka_has_alias(videoMode.resolution))
 *     {
 *         kc_set_resolution(ka_aliased(videoMode.resolution));
 *     }
 *     else
 *     {
 *         ev_new_video_mode.fire(videoMode);
 *     }
 * });
 * @endcode
 *
 * @see
 * ev_new_video_mode, kc_set_resolution()
 */
extern vcs_event_c<const video_mode_s&> ev_new_proposed_video_mode;

/*!
 * Fired when the capture subsystem has begun capturing in a new video mode.
 *
 * It's not guaranteed that the new video mode is different from the previous
 * one, although usually it will be.
 *
 * @see
 * ev_new_proposed_video_mode, kc_set_resolution()
 */
extern vcs_event_c<const video_mode_s&> ev_new_video_mode;

/*!
 * Fired when the capture device is switched to a different input channel.
 */
extern vcs_event_c<unsigned> ev_new_input_channel;

/*!
 * Fired when the capture subsystem reports its capture device to be invalid in a
 * way that renders the device unusable to the subsystem.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the device is detected to be invalid, but rather once VCS has polled the capture
 * subsystem (via kc_process_next_capture_event()) to find that such is the case.
 */
extern vcs_event_c<void> ev_invalid_capture_device;

/*!
 * Fired when the capture device loses its input signal. This event implies that
 * the device was previously receiving a signal.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the signal is lost, but rather once VCS has polled the capture subsystem (via
 * kc_process_next_capture_event()) to find that such is the case.
 *
 * @code
 * // Print a message every time the capture signal is lost.
 * ev_capture_signal_lost.listen([]
 * {
 *     printf("The signal was lost.\n");
 * });
 * @endcode
 *
 * @see
 * ev_capture_signal_gained
 */

extern vcs_event_c<void> ev_capture_signal_lost;

/*!
 * Fired when the capture device begins receiving an input signal. This event
 * implies that the device was previously not receiving a signal.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact time
 * the signal is gained, but rather once VCS has polled the capture subsystem (via
 * kc_process_next_capture_event()) to find that such is the case.
 *
 * @see
 * ev_capture_signal_lost
 */
extern vcs_event_c<void> ev_capture_signal_gained;

/*!
 * Fired when the capture device reports its input signal to be invalid (e.g.
 * out of range).
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time the invalid signal began to be received, but rather once VCS has polled
 * the capture subsystem (via kc_process_next_capture_event()) to find that such a
 * condition exists.
 */
extern vcs_event_c<void> ev_invalid_capture_signal;

/*!
 * Fired when an error occurs in the capture subsystem from which the
 * subsystem can't recover.
 *
 * The event is fired by VCS's event loop rather than directly by the capture
 * subsystem. As such, the event isn't guaranteed to be fired at the exact
 * time the error occurs, but rather once VCS has polled the capture subsystem
 * (via kc_process_next_capture_event()) to find that there has been such an error.
 */
extern vcs_event_c<void> ev_unrecoverable_capture_error;


/*!
 * An event that can be fired by subsystems to indicate that the output window should
 * redraw its contents.
 */
extern vcs_event_c<void> ev_dirty_output_window;

/*!
 * Fired when the scaler subsystem scales a frame into a resolution
 * different from the previous frame's.
 *
 * @code
 * ks_scale_frame(frame);
 *
 * ks_set_scaling_multiplier(ks_scaling_multiplier() + 1);
 *
 * // Fires ks_ev_new_output_resolution.
 * ks_scale_frame(frame);
 * @endcode
 */
extern vcs_event_c<const resolution_s&> ev_new_output_resolution;

/*!
 * Fired when the scaler subsystem has finished scaling a frame.
 *
 * @code
 * ev_new_output_image.listen([](const image_s &scaledImage)
 * {
 *     printf("Scaled a frame to %lu x %lu.\n", scaledImage.resolution.w, scaledImage.resolution.h);
 * });
 * @endcode
 *
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<const image_s&> ev_new_output_image;

/*!
 * Fired once per second, giving the number of input frames the scaler
 * subsystem scaled during that time.
 *
 * @code
 * ev_frames_per_second.listen([](unsigned numFrames)
 * {
 *     printf("Scaled %u frames per second.\n", numFrames);
 * });
 * @endcode
 *
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<unsigned> ev_frames_per_second;

/*!
 * Fired when an output scaling filter becomes active, i.e. when captured
 * frames begin being scaled using such a filter.
 */
extern vcs_event_c<void> ev_custom_output_scaler_enabled;

/*!
 * Fired when there's no longer an output scaling filter being used to
 * scale captured frames.
 */
extern vcs_event_c<void> ev_custom_output_scaler_disabled;

extern vcs_event_c<void> ev_eco_mode_enabled;
extern vcs_event_c<void> ev_eco_mode_disabled;
extern vcs_event_c<const captured_frame_s&> ev_frame_processing_finished;
extern vcs_event_c<const video_preset_s*> ev_video_preset_activated;

#endif
