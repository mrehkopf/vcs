/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*
 * App-wide event system.
 *
 * VCS's application-wide event system provides a framework for the various subsystems
 * of the application to receive notification of program events.
 *
 * ## Usage
 *
 * Simply subscribe a listener function to the event you're interested in being
 * notified of:
 *
 *     ev_frames_per_second.listen([](unsigned fps)
 *     {
 *         printf("Capturing at %u FPS\n", fps);
 *     });
 *
 *     // The listener will execute whenever ev_frames_per_second.fire() is invoked.
 *
 * If there are multiple listeners to an event, they'll be called synchronously in the
 * order they were subscribed.
 *
 * Note: Listener functions can't be unsubscribed.
 */

#ifndef VCS_COMMON_VCS_EVENT_VCS_EVENT_H
#define VCS_COMMON_VCS_EVENT_VCS_EVENT_H

#include <functional>
#include <list>

struct captured_frame_s;
struct refresh_rate_s;
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

// Fired when the capture subsystem makes a new captured frame available. A
// reference to the frame's data is provided; a listener can assume that the
// data will remain valid for the duration of the callback, but should make a
// local copy if later reference is needed.
extern vcs_event_c<const captured_frame_s&> ev_new_captured_frame;

// Fired when the capture device reports that the video mode of the currently-
// captured  signal has changed.
//
// This event is to be treated as a proposal from the capture device that the
// given video mode now best fits the captured signal. You can accept the
// proposal by firing the ev_new_video_mode event, call kc_set_resolution()
// to force the capture device to use a different mode, or do nothing to retain
// the existing mode.
extern vcs_event_c<const video_mode_s&> ev_new_proposed_video_mode;

// Fired when the capture subsystem has begun capturing in a new video mode.
extern vcs_event_c<const video_mode_s&> ev_new_video_mode;

// Fired when the capture device is switched to a different input channel.
extern vcs_event_c<unsigned> ev_new_input_channel;

// Fired when the capture subsystem reports its capture device to be invalid in a
// way that renders the device unusable to the subsystem.
extern vcs_event_c<void> ev_invalid_capture_device;

// Fired when the capture device loses its input signal. This event implies that
// the device was previously receiving a signal.
extern vcs_event_c<void> ev_capture_signal_lost;

// Fired when the capture device begins receiving an input signal. This event
// implies that the device was previously not receiving a signal.
extern vcs_event_c<void> ev_capture_signal_gained;

// Fired when the capture device reports its input signal to be invalid (e.g.
// out of range).
extern vcs_event_c<void> ev_invalid_capture_signal;

// An event that can be fired by subsystems to indicate that the output window
// should redraw its contents.
extern vcs_event_c<void> ev_dirty_output_window;

// Fired when the scaler subsystem scales a frame into a resolution
// different from the previous frame's.
extern vcs_event_c<const resolution_s&> ev_new_output_resolution;

// Fired when the scaler subsystem has finished scaling a frame.
extern vcs_event_c<const image_s&> ev_new_output_image;

// Fired roughly once per second, giving the number of input frames the scaler
// subsystem scaled during that time.
extern vcs_event_c<const refresh_rate_s&> ev_frames_per_second;

// Fired when an output scaling filter becomes active, i.e. when captured
// frames begin being scaled using such a filter.
extern vcs_event_c<void> ev_custom_output_scaler_enabled;

// Fired when there's no longer an output scaling filter being used to
// scale captured frames.
extern vcs_event_c<void> ev_custom_output_scaler_disabled;

extern vcs_event_c<void> ev_eco_mode_enabled;
extern vcs_event_c<void> ev_eco_mode_disabled;
extern vcs_event_c<const captured_frame_s&> ev_frame_processing_finished;
extern vcs_event_c<const video_preset_s*> ev_video_preset_activated;
extern vcs_event_c<const video_preset_s*> ev_video_preset_name_changed;
extern vcs_event_c<unsigned> ev_missed_frames_count;
extern vcs_event_c<unsigned> ev_capture_processing_latency;
extern vcs_event_c<const refresh_rate_s&> ev_new_capture_rate;

#endif
