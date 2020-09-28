/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * The app event system allows the various subsystems of VCS to receive notification
 * when certain runtime events occur.
 *
 * A subsystem wishing to be notified of a particular event will subscribe a callback
 * function to that event; e.g. "ke_events().capture.newFrame->subscribe([]{...})".
 * The callback function will then be notified whenever that event occurs.
 *
 */

#ifndef EVENT_H
#define EVENT_H

#include <functional>
#include <list>

typedef std::function<void(void)> vcs_event_handler_fn_t;

class vcs_event_c
{
public:
    // Ask the given handler function to be called when this event fires.
    void subscribe(vcs_event_handler_fn_t handlerFn);

    // Fire the event, causing all subscribed handler functions to be called
    // immediately, in the order of their subscription.
    void fire(void) const;

private:
    std::list<vcs_event_handler_fn_t> subscribedHandlers;
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
        vcs_event_c *const newFrame = new vcs_event_c;

        // The capture device has received a new video mode. We treat it as a proposal,
        // since we might e.g. not want this video mode to be used, and in that case
        // would tell the capture device to use some other mode.
        vcs_event_c *const newProposedVideoMode = new vcs_event_c;

        // The capture device has received a new video mode that we've approved of
        // (cf. newProposedVideoMode).
        vcs_event_c *const newVideoMode = new vcs_event_c;

        // The active input channel index has changed.
        vcs_event_c *const newInputChannel = new vcs_event_c;

        vcs_event_c *const signalLost = new vcs_event_c;
        vcs_event_c *const signalGained = new vcs_event_c;
        vcs_event_c *const invalidSignal = new vcs_event_c;
        vcs_event_c *const unrecoverableError = new vcs_event_c;
    } capture;

    // Events related to video recording.
    struct
    {
        vcs_event_c *const recordingStarted = new vcs_event_c;
        vcs_event_c *const recordingEnded = new vcs_event_c;
    } recorder;

    // Events related to file IO.
    struct
    {
        vcs_event_c *const savedVideoPresets = new vcs_event_c;
        vcs_event_c *const savedFilterGraph = new vcs_event_c;
        vcs_event_c *const savedAliases = new vcs_event_c;
        vcs_event_c *const loadedVideoPresets = new vcs_event_c;
        vcs_event_c *const loadedFilterGraph = new vcs_event_c;
        vcs_event_c *const loadedAliases = new vcs_event_c;
    } file;

    // Events related to the GUI.
    struct
    {
        // Marks the output window as dirty, i.e. in need of redrawing.
        vcs_event_c *const dirty = new vcs_event_c;
    } display;

    struct
    {
        vcs_event_c *const newFrameResolution = new vcs_event_c;
        vcs_event_c *const newFrame = new vcs_event_c;
    } scaler;

private:
};

vcs_event_pool_c& ke_events(void);

#endif
