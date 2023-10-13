/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "capture/capture.h"
#include "common/timer/timer.h"

static std::mutex CAPTURE_MUTEX;

std::mutex& kc_mutex(void)
{
    return CAPTURE_MUTEX;
}

subsystem_releaser_t kc_initialize_capture(void)
{
    DEBUG(("Initializing the capture subsystem."));

    kc_initialize_device();

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
