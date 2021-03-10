/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * The app event system allows the various subsystems of VCS to receive notification
 * when certain runtime events occur.
 *
 * A subsystem wishing to be notified of a particular event will subscribe a callback
 * function to that event; e.g. "ke_events().capture.newFrame.subscribe([]{...})".
 * The callback function will then be notified whenever that event occurs.
 *
 */

#ifndef VCS_COMMON_PROPAGATE_APP_EVENTS_H
#define VCS_COMMON_PROPAGATE_APP_EVENTS_H

#include <functional>
#include <list>

// An event that passes no parameters.
class vcs_event_c
{
public:
    // Ask the given handler function to be called when this event fires.
    void subscribe(std::function<void(void)> handlerFn);

    // Fire the event, causing all subscribed handler functions to be called
    // immediately, in the order of their subscription.
    void fire(void) const;

private:
    std::list<std::function<void(void)>> subscribedHandlers;
};

// Same as vcs_event_c, but passes on a type T value.
template <typename T>
class vcs_event_with_value_c
{
public:
    void subscribe(std::function<void(T)> handlerFn)
    {
        this->subscribedHandlers.push_back(handlerFn);

        return;
    }

    void fire(T value) const
    {
        for (const auto &handlerFn: this->subscribedHandlers)
        {
            handlerFn(value);
        }

        return;
    }

private:
    std::list<std::function<void(T)>> subscribedHandlers;
};

// The selection of events recognized by VCS.
class vcs_event_pool_c
{
public:
    // Events related to capturing.
    struct
    {
        // VCS has received a new frame from the capture API. (The frame's data is
        // available from kc_capture_api().get_frame_buffer().)
        vcs_event_c newFrame;

        // The capture device has received a new video mode. We treat it as a proposal,
        // since we might e.g. not want this video mode to be used, and in that case
        // would tell the capture device to use some other mode.
        vcs_event_c newProposedVideoMode;

        // The capture device has received a new video mode that we've approved of
        // (cf. newProposedVideoMode).
        vcs_event_c newVideoMode;

        // The active input channel index has changed.
        vcs_event_c newInputChannel;

        // The current capture device is invalid.
        vcs_event_c invalidDevice;

        vcs_event_c signalLost;
        vcs_event_c signalGained;
        vcs_event_c invalidSignal;
        vcs_event_c unrecoverableError;

        // The capture subsystem has had to ignore frames coming from the capture
        // device because VCS was busy with something else (e.g. with processing
        // a previous frame). Provides the count of missed frames (generally, the
        // capture subsystem might fire this event at regular intervals and pass
        // the count of missed frames during that interval).
        vcs_event_with_value_c<unsigned> missedFramesCount;
    } capture;

    // Events related to video recording.
    struct
    {
        vcs_event_c recordingStarted;
        vcs_event_c recordingEnded;
    } recorder;

    // Events related to file IO.
    struct
    {
        vcs_event_c savedVideoPresets;
        vcs_event_c savedFilterGraph;
        vcs_event_c savedAliases;
        vcs_event_c loadedVideoPresets;
        vcs_event_c loadedFilterGraph;
        vcs_event_c loadedAliases;
    } file;

    // Events related to the GUI.
    struct
    {
        // Marks the output window as dirty, i.e. in need of redrawing.
        vcs_event_c dirty;
    } display;

    struct
    {
        vcs_event_c newFrameResolution;

        // The most recent captured frame has now been processed and is ready for display.
        vcs_event_c newFrame;

        // The number of frames processed (see newFrame) in the last second.
        vcs_event_with_value_c<unsigned> framesPerSecond;
    } scaler;

private:
};

vcs_event_pool_c& ke_events(void);

#endif
