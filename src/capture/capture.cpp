/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "common/timer/timer.h"

vcs_event_c<const captured_frame_s&> kc_ev_new_captured_frame;
vcs_event_c<const video_mode_s&> kc_ev_new_proposed_video_mode;
vcs_event_c<const video_mode_s&> kc_ev_new_video_mode;
vcs_event_c<unsigned> kc_ev_input_channel_changed;
vcs_event_c<void> kc_ev_invalid_device;
vcs_event_c<void> kc_ev_signal_lost;
vcs_event_c<void> kc_ev_signal_gained;
vcs_event_c<void> kc_ev_invalid_signal;
vcs_event_c<void> kc_ev_unrecoverable_error;

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

video_signal_parameters_s video_signal_parameters_s::from_capture_device(const std::string &nameSpace)
{
    return video_signal_parameters_s{
        .brightness         = long(kc_device_property("brightness" + nameSpace)),
        .contrast           = long(kc_device_property("contrast" + nameSpace)),
        .redBrightness      = long(kc_device_property("red brightness" + nameSpace)),
        .redContrast        = long(kc_device_property("red contrast" + nameSpace)),
        .greenBrightness    = long(kc_device_property("green brightness" + nameSpace)),
        .greenContrast      = long(kc_device_property("green contrast" + nameSpace)),
        .blueBrightness     = long(kc_device_property("blue brightness" + nameSpace)),
        .blueContrast       = long(kc_device_property("blue contrast" + nameSpace)),
        .horizontalSize     = ulong(kc_device_property("horizontal size" + nameSpace)),
        .horizontalPosition = long(kc_device_property("horizontal position" + nameSpace)),
        .verticalPosition   = long(kc_device_property("vertical position" + nameSpace)),
        .phase              = long(kc_device_property("phase" + nameSpace)),
        .blackLevel         = long(kc_device_property("black level" + nameSpace))
    };
}

void video_signal_parameters_s::to_capture_device(const video_signal_parameters_s &params, const std::string &nameSpace)
{
    kc_set_device_property(("brightness" + nameSpace), params.brightness);
    kc_set_device_property(("contrast" + nameSpace), params.contrast);
    kc_set_device_property(("red brightness" + nameSpace), params.redBrightness);
    kc_set_device_property(("green brightness" + nameSpace), params.greenBrightness);
    kc_set_device_property(("blue brightness" + nameSpace), params.blueBrightness);
    kc_set_device_property(("red contrast" + nameSpace), params.redContrast);
    kc_set_device_property(("green contrast" + nameSpace), params.greenContrast);
    kc_set_device_property(("blue contrast" + nameSpace), params.blueContrast);
    kc_set_device_property(("horizontal size" + nameSpace), params.horizontalSize);
    kc_set_device_property(("horizontal position" + nameSpace), params.horizontalPosition);
    kc_set_device_property(("vertical position" + nameSpace), params.verticalPosition);
    kc_set_device_property(("phase" + nameSpace), params.phase);
    kc_set_device_property(("black level" + nameSpace), params.blackLevel);

    return;
}
