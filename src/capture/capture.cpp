/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS capture
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "common/timer/timer.h"

vcs_event_c<void> kcEvent_frameCaptured;
vcs_event_c<void> kcEvent_newProposedVideoMode;
vcs_event_c<void> kcEvent_newVideoMode;
vcs_event_c<void> kcEvent_newInputChannel;
vcs_event_c<void> kcEvent_invalidDevice;
vcs_event_c<void> kcEvent_signalLost;
vcs_event_c<void> kcEvent_signalGained;
vcs_event_c<void> kcEvent_invalidSignal;
vcs_event_c<void> kcEvent_unrecoverableError;
vcs_event_c<unsigned> kcEvent_missedFramesCount;

// We'll keep a running count of the number of frames we've missed, in total,
// during capturing.
static unsigned LAST_KNOWN_MISSED_FRAMES_COUNT = 0;

static std::mutex CAPTURE_MUTEX;

std::mutex& kc_capture_mutex(void)
{
    return CAPTURE_MUTEX;
}

void kc_initialize_capture(void)
{
     INFO(("Initializing the capture subsystem."));

    kc_initialize_device();

    kt_timer(1000, [](const unsigned)
    {
        const unsigned numMissedCurrent = kc_get_missed_frames_count();
        const unsigned numMissedFrames = (numMissedCurrent- LAST_KNOWN_MISSED_FRAMES_COUNT);

        LAST_KNOWN_MISSED_FRAMES_COUNT = numMissedCurrent;

        kcEvent_missedFramesCount.fire(numMissedFrames);
    });

    return;
}

void kc_release_capture(void)
{
    INFO(("Releasing the capture subsystem."));

    kc_release_device();

    return;
}

bool kc_force_capture_resolution(const resolution_s &r)
{
    #if CAPTURE_DEVICE_VISION_V4L
        NBENE(("Custom input resolutions are not supported with Video4Linux."));
        return false;
    #endif

    const resolution_s min = kc_get_device_minimum_resolution();
    const resolution_s max = kc_get_device_maximum_resolution();

    if (kc_has_no_signal())
    {
        DEBUG(("Was asked to change the input resolution while the capture card was not receiving a signal. Ignoring the request."));
        return false;
    }

    if (r.w > max.w ||
        r.w < min.w ||
        r.h > max.h ||
        r.h < min.h)
    {
        NBENE(("Was asked to set an unsupported input resolution (%u x %u). Ignoring the request.",
               r.w, r.h));
        return false;
    }

    if (!kc_set_capture_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        return false;
    }

    kcEvent_newVideoMode.fire();

    return true;
}
