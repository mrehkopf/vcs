/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS capture
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "common/propagate/app_events.h"
#include "capture/capture_device_virtual.h"
#include "capture/capture_device_rgbeasy.h"
#include "capture/capture_device_vision_v4l.h"
#include "capture/capture.h"
#include "common/timer/timer.h"

static capture_device_s *API = nullptr;

// We'll keep a running count of the number of frames we've missed, in total,
// during capturing.
static unsigned LAST_KNOWN_MISSED_FRAMES_COUNT = 0;

capture_device_s& kc_capture_device(void)
{
    k_assert(API, "Attempting to fetch the capture API prior to its initialization.");
    
    return *API;
}

void kc_initialize_capture(void)
{
    API =
    #ifdef CAPTURE_DEVICE_VIRTUAL
        new capture_device_virtual_s;
    #elif CAPTURE_DEVICE_RGBEASY
        new capture_device_rgbeasy_s;
    #elif CAPTURE_DEVICE_VISION_V4L
        new capture_device_vision_v4l_s;
    #else
        #error "Unknown capture API."
    #endif

    API->initialize();

    kt_timer(1000, [](const unsigned)
    {
        const unsigned numMissedCurrent = kc_capture_device().get_missed_frames_count();
        const unsigned numMissedFrames = (numMissedCurrent- LAST_KNOWN_MISSED_FRAMES_COUNT);

        LAST_KNOWN_MISSED_FRAMES_COUNT = numMissedCurrent;

        ke_events().capture.missedFramesCount.fire(numMissedFrames);
    });

    return;
}

void kc_release_capture(void)
{
    INFO(("Releasing the capture API."));

    API->release();

    delete API;
    API = nullptr;

    return;
}

bool kc_force_input_resolution(const resolution_s &r)
{
    #if CAPTURE_DEVICE_VISION_V4L
        NBENE(("Custom input resolutions are not supported with Video4Linux."));
        return false;
    #endif

    const resolution_s min = kc_capture_device().get_minimum_resolution();
    const resolution_s max = kc_capture_device().get_maximum_resolution();

    if (kc_capture_device().has_no_signal())
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

    if (!kc_capture_device().set_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        return false;
    }

    ke_events().capture.newVideoMode.fire();

    return true;
}
