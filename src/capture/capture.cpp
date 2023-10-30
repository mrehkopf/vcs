/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include "capture/capture.h"
#include "common/timer/timer.h"

static std::mutex CAPTURE_MUTEX;

static unsigned LAST_KNOWN_MISSED_FRAMES_COUNT = 0;

std::mutex& kc_mutex(void)
{
    return CAPTURE_MUTEX;
}

subsystem_releaser_t kc_initialize_capture(void)
{
    DEBUG(("Initializing the capture subsystem."));

    kc_initialize_device();

    kt_timer(1000, [](const unsigned)
    {
        const unsigned numMissedCurrent = kc_dropped_frames_count();
        const unsigned numMissedFrames = (numMissedCurrent- LAST_KNOWN_MISSED_FRAMES_COUNT);

        LAST_KNOWN_MISSED_FRAMES_COUNT = numMissedCurrent;
        ev_missed_frames_count.fire(numMissedFrames);
    });

    return []{
        DEBUG(("Releasing the capture subsystem."));
        kc_release_device();
    };
}

bool kc_has_signal(void)
{
    return kc_device_property("has signal");
}

const std::vector<const char*>& kc_supported_video_preset_properties(void)
{
    static const std::vector<const char*> emptyList;
    const auto *supportedProps = ((std::vector<const char*>*)kc_device_property("supported video preset properties"));
    return (supportedProps? *supportedProps : emptyList);
}

refresh_rate_s capture_rate_s::from_capture_device_properties(void)
{
    return refresh_rate_s::from_fixedpoint(kc_device_property("capture rate"));
}

void capture_rate_s::to_capture_device_properties(const refresh_rate_s &rate)
{
    kc_set_device_property("capture rate", rate.fixedpoint);
}
