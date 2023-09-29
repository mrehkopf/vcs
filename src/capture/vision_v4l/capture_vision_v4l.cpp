/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * Implements capturing under Linux via the Datapath's Vision driver and the
 * Vision4Linux 2 API.
 *
 */

#include <unordered_map>
#include <cmath>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <chrono>
#include <poll.h>
#include "capture/vision_v4l/input_channel_v4l.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "capture/vision_v4l/ic_v4l_video_parameters.h"
#include "common/vcs_event/vcs_event.h"
#include "main.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133control.h>
#include <visionrgb/include/rgb133v4l2.h>

// The input channel (/dev/videoX device) we're currently capturing from.
static input_channel_v4l_c *INPUT_CHANNEL = nullptr;

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// Cumulative count of frames that were sent to us by the capture device but which
// VCS was too busy to process. Note that this count doesn't account for the missed
// frames on the current input channel, only on previous ones. The total number of
// missed frames over the program's execution is thus this value + the current input
// channel's value.
static unsigned NUM_MISSED_FRAMES = 0;

// Set to true if we've been asked to force a particular capture resolution. Will
// be reset to false once the resolution has been forced.
static bool FORCE_CUSTOM_RESOLUTION = false;

static std::unordered_map<std::string, double> DEVICE_PROPERTIES = {
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},

    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"supports channel switching: ui", true},
    {"supports resolution switching: ui", true},
};

static bool force_capture_resolution(const resolution_s r)
{
    k_assert(INPUT_CHANNEL, "Attempting to set the capture resolution on a null input channel.");
    k_assert(FORCE_CUSTOM_RESOLUTION, "Attempting to force the capture resolution without it having been requested.");

    FORCE_CUSTOM_RESOLUTION = false;

    if (!kc_has_signal())
    {
        NBENE(("Cannot set the capture resolution while there is no signal."));
        return false;
    }

    // Validate the resolution.
    {
        const auto minRes = resolution_s::from_capture_device_properties(": minimum");
        const auto maxRes = resolution_s::from_capture_device_properties(": maximum");

        if (
            (r.w < minRes.w) ||
            (r.w > maxRes.w) ||
            (r.h < minRes.h) ||
            (r.h > maxRes.h)
        ){
            NBENE(("Failed to set the capture resolution: out of range"));
            goto fail;
        }
    }

    // Set the resolution.
    {
        v4l2_control control = {0};

        control.id = RGB133_V4L2_CID_HOR_TIME;
        control.value = r.w;
        if (!INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture width: device error."));
            return false;
        }

        control.id = RGB133_V4L2_CID_VER_TIME;
        control.value = r.h;
        if (!INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture height: device error."));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

static void set_input_channel(const unsigned channelIdx)
{
    if (INPUT_CHANNEL)
    {
        NUM_MISSED_FRAMES += INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped;
        delete INPUT_CHANNEL;
    }

    INPUT_CHANNEL = new input_channel_v4l_c(
        (std::string("/dev/video") + std::to_string(unsigned(channelIdx))),
        3,
        &FRAME_BUFFER
    );

    ev_new_input_channel.fire(channelIdx);

    return;
}

double kc_device_property(const std::string &key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string &key, double value)
{
    if (key == "width")
    {
        if (
            (value > MAX_CAPTURE_WIDTH) ||
            (value < MIN_CAPTURE_WIDTH)
        ){
            return false;
        }

        if (value != INPUT_CHANNEL->captureStatus.resolution.w)
        {
            FORCE_CUSTOM_RESOLUTION = true;
        }
        else
        {
            FRAME_BUFFER.resolution.w = value;
        }
    }
    else if (key == "height")
    {
        if (
            (value > MAX_CAPTURE_HEIGHT) ||
            (value < MIN_CAPTURE_HEIGHT)
        ){
            return false;
        }

        if (value != INPUT_CHANNEL->captureStatus.resolution.h)
        {
            FORCE_CUSTOM_RESOLUTION = true;
        }
        else
        {
            FRAME_BUFFER.resolution.h = value;
        }
    }
    else if (key == "channel")
    {
        set_input_channel(value);
    }
    // Video parameters.
    else
    {
        static const auto set_video_param = [](const int value, const ic_v4l_controls_c::type_e parameterType)->bool
        {
            if (!kc_has_signal())
            {
                DEBUG(("Was asked to set capture video params while there was no signal. Ignoring the request."));
                return false;
            }

            return INPUT_CHANNEL->captureStatus.videoParameters.set_value(value, parameterType);
        };

        static const auto videoParams = std::vector<std::pair<std::string, ic_v4l_controls_c::type_e>>{
            {"brightness",          ic_v4l_controls_c::type_e::brightness},
            {"contrast",            ic_v4l_controls_c::type_e::contrast},
            {"red brightness",      ic_v4l_controls_c::type_e::red_brightness},
            {"green brightness",    ic_v4l_controls_c::type_e::green_brightness},
            {"blue brightness",     ic_v4l_controls_c::type_e::blue_brightness},
            {"red contrast",        ic_v4l_controls_c::type_e::red_contrast},
            {"green contrast",      ic_v4l_controls_c::type_e::green_contrast},
            {"blue contrast",       ic_v4l_controls_c::type_e::blue_contrast},
            {"horizontal size",     ic_v4l_controls_c::type_e::horizontal_size},
            {"horizontal position", ic_v4l_controls_c::type_e::horizontal_position},
            {"vertical position",   ic_v4l_controls_c::type_e::vertical_position},
            {"phase",               ic_v4l_controls_c::type_e::phase},
            {"black level",         ic_v4l_controls_c::type_e::black_level},
        };

        for (const auto &videoParam: videoParams)
        {
            if (key == videoParam.first)
            {
                set_video_param(value, videoParam.second);
                break;
            }
        }
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

capture_event_e kc_process_next_capture_event(void)
{
    if (!INPUT_CHANNEL)
    {
        ev_unrecoverable_capture_error.fire();
        return capture_event_e::unrecoverable_error;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::unrecoverable_error))
    {
        INPUT_CHANNEL->captureStatus.invalidDevice = true;
        ev_unrecoverable_capture_error.fire();
        return capture_event_e::unrecoverable_error;
    }
    else if (FORCE_CUSTOM_RESOLUTION)
    {
        force_capture_resolution(resolution_s{
            .w = unsigned(kc_device_property("width")),
            .h = unsigned(kc_device_property("height"))
        });
        return capture_event_e::none;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::new_video_mode))
    {
        kc_set_device_property("width", INPUT_CHANNEL->captureStatus.resolution.w);
        kc_set_device_property("height", INPUT_CHANNEL->captureStatus.resolution.h);
        kc_set_device_property("refresh rate", INPUT_CHANNEL->captureStatus.refreshRate.value<double>());

        // Re-create the input channel for the new video mode.
        kc_set_device_property("channel", kc_device_property("channel"));

        return capture_event_e::new_video_mode;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_lost))
    {
        kc_set_device_property("has signal", false);
        ev_capture_signal_lost.fire();
        return capture_event_e::signal_lost;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_gained))
    {
        kc_set_device_property("has signal", true);
        ev_capture_signal_gained.fire();
        return capture_event_e::signal_gained;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_signal))
    {
        kc_set_device_property("has signal", false);
        ev_invalid_capture_signal.fire();
        return capture_event_e::invalid_signal;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_device))
    {
        ev_invalid_capture_device.fire();
        return capture_event_e::invalid_device;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(FRAME_BUFFER);
        return capture_event_e::new_frame;
    }
    else if (INPUT_CHANNEL->pop_capture_event(capture_event_e::sleep))
    {
        return capture_event_e::sleep;
    }

    return capture_event_e::none;
}

uint kc_dropped_frames_count(void)
{
    k_assert(INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return (NUM_MISSED_FRAMES + INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped);
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the Vision/V4L capture device."));

    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    kc_set_device_property("channel", INPUT_CHANNEL_IDX);
    k_assert(INPUT_CHANNEL, "Failed to initialize the hardware input channel.");

    resolution_s::to_capture_device_properties({.w = 640, .h = 480});

    // Initialize value ranges for video parameters.
    //
    // Note: Querying these values from V4L at runtime seems to work a little
    // unreliably, so for now let's just use hard-coded values. These values
    // have been selected with the VisionRGB-E1S in mind.
    {
        ic_v4l_controls_c *const videoParams = &INPUT_CHANNEL->captureStatus.videoParameters;

        videoParams->update();

        if (!kc_has_signal())
        {
            video_signal_properties_s::to_capture_device_properties(video_signal_properties_s{
                .brightness         = 32,
                .contrast           = 128,
                .redBrightness      = 128,
                .redContrast        = 256,
                .greenBrightness    = 128,
                .greenContrast      = 256,
                .blueBrightness     = 128,
                .blueContrast       = 256,
                .horizontalSize     = 900,
                .horizontalPosition = 112,
                .verticalPosition   = 36,
                .phase              = 0,
                .blackLevel         = 8
            }, ": default");
        }
        else
        {
            video_signal_properties_s::to_capture_device_properties({
                .brightness         = videoParams->default_value(ic_v4l_controls_c::type_e::brightness),
                .contrast           = videoParams->default_value(ic_v4l_controls_c::type_e::contrast),
                .redBrightness      = videoParams->default_value(ic_v4l_controls_c::type_e::red_brightness),
                .redContrast        = videoParams->default_value(ic_v4l_controls_c::type_e::red_contrast),
                .greenBrightness    = videoParams->default_value(ic_v4l_controls_c::type_e::green_brightness),
                .greenContrast      = videoParams->default_value(ic_v4l_controls_c::type_e::green_contrast),
                .blueBrightness     = videoParams->default_value(ic_v4l_controls_c::type_e::blue_brightness),
                .blueContrast       = videoParams->default_value(ic_v4l_controls_c::type_e::blue_contrast),
                .horizontalSize     = unsigned(videoParams->default_value(ic_v4l_controls_c::type_e::horizontal_size)),
                .horizontalPosition = videoParams->default_value(ic_v4l_controls_c::type_e::horizontal_position),
                .verticalPosition   = videoParams->default_value(ic_v4l_controls_c::type_e::vertical_position),
                .phase              = videoParams->default_value(ic_v4l_controls_c::type_e::phase),
                .blackLevel         = videoParams->default_value(ic_v4l_controls_c::type_e::black_level)
            }, ": default");
        }

        video_signal_properties_s::to_capture_device_properties({
            .brightness         = 0,
            .contrast           = 0,
            .redBrightness      = 0,
            .redContrast        = 0,
            .greenBrightness    = 0,
            .greenContrast      = 0,
            .blueBrightness     = 0,
            .blueContrast       = 0,
            .horizontalSize     = 100,
            .horizontalPosition = 1,
            .verticalPosition   = 1,
            .phase              = 0,
            .blackLevel         = 1
        }, ": minimum");

        video_signal_properties_s::to_capture_device_properties({
            .brightness         = 63,
            .contrast           = 255,
            .redBrightness      = 255,
            .redContrast        = 511,
            .greenBrightness    = 255,
            .greenContrast      = 511,
            .blueBrightness     = 255,
            .blueContrast       = 511,
            .horizontalSize     = 4095,
            .horizontalPosition = 1200,
            .verticalPosition   = 63,
            .phase              = 31,
            .blackLevel         = 255
        }, ": maximum");
    }

    // Listen for relevant events.
    {
        ev_capture_signal_gained.listen([]
        {
            k_assert(INPUT_CHANNEL, "Attempting to set input channel parameters on a null channel.");

            INPUT_CHANNEL->captureStatus.videoParameters.update();

            ev_new_proposed_video_mode.fire({
                INPUT_CHANNEL->captureStatus.resolution,
                INPUT_CHANNEL->captureStatus.refreshRate,
            });
        });

        ev_frame_processing_finished.listen([]
        {
            k_assert(INPUT_CHANNEL, "Attempting to set input channel parameters on a null channel.");
            INPUT_CHANNEL->captureStatus.numFramesProcessed++;
        });
    }

    // Start capturing.
    kc_set_device_property("channel", INPUT_CHANNEL_IDX);
    k_assert(INPUT_CHANNEL, "Failed to initialize the input channel.");

    return;
}

bool kc_release_device(void)
{
    delete INPUT_CHANNEL;
    delete [] FRAME_BUFFER.pixels;

    return true;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return FRAME_BUFFER;
}
