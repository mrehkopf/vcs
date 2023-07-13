/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QColor>
#include <unordered_map>
#include <chrono>
#include "common/globals.h"
#include "common/propagate/vcs_event.h"
#include "common/timer/timer.h"
#include "capture/capture.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static captured_frame_s FRAME_BUFFER;

static std::unordered_map<std::string, double> DEVICE_PROPERTIES = {
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},

    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"has signal", true},

    {"supports channel switching: ui", true},
    {"supports resolution switching: ui", true},
};

static void redraw_test_pattern(void)
{
    static unsigned numTicks = 0;

    numTicks++;
    NUM_FRAMES_PER_SECOND++;

    for (unsigned y = 0; y < FRAME_BUFFER.r.h; y++)
    {
        for (unsigned x = 0; x < FRAME_BUFFER.r.w; x++)
        {
            /// TODO: Remove the dependency on Qt for the RGB -> HSL -> RGB conversion.
            QColor rgbGradient;
            const double widthRatio = (((x + numTicks) % FRAME_BUFFER.r.w) / double(FRAME_BUFFER.r.w));
            const double heightRatio = (y / double(FRAME_BUFFER.r.h));
            rgbGradient.setHsl((widthRatio * 359), 255, (255 - (heightRatio * 255)));
            rgbGradient = rgbGradient.toRgb();

            const u8 red = rgbGradient.red();
            const u8 green = rgbGradient.green();
            const u8 blue = rgbGradient.blue();
            const u8 alpha = 255;

            const unsigned idx = ((x + y * FRAME_BUFFER.r.w) * (FRAME_BUFFER.r.bpp / 8));
            FRAME_BUFFER.pixels[idx + 0] = blue;
            FRAME_BUFFER.pixels[idx + 1] = green;
            FRAME_BUFFER.pixels[idx + 2] = red;
            FRAME_BUFFER.pixels[idx + 3] = alpha;
        }
    }

    return;
}

static void push_capture_event(const capture_event_e event)
{
    CAPTURE_EVENT_FLAGS[(int)event] = true;

    return;
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the virtual capture device."));

    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");
    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    kc_set_device_property("input channel index", INPUT_CHANNEL_IDX);
    kc_set_device_property("width", FRAME_BUFFER.r.w);
    kc_set_device_property("height", FRAME_BUFFER.r.h);

    // Simulate the capturing of a new frame.
    kt_timer(std::round(1000 / TARGET_REFRESH_RATE), [](const unsigned)
    {
        const auto inres = resolution_s::from_capture_device();

        if ((inres.w > MAX_CAPTURE_WIDTH) || (inres.h > MAX_CAPTURE_HEIGHT))
        {
            kc_set_device_property("has signal", false);
        }
        else
        {
            kc_set_device_property("has signal", true);
            redraw_test_pattern();
            push_capture_event(capture_event_e::new_frame);
        }
    });

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        const double newRefreshRate = (NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs));

        NUM_FRAMES_PER_SECOND = 0;

        if (kc_device_property("refresh rate") != newRefreshRate)
        {
            kc_set_device_property("refresh rate", newRefreshRate);
            push_capture_event(capture_event_e::new_video_mode);
        }
    });

    return;
}

double kc_device_property(const std::string key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string key, double value)
{
    if (key == "width")
    {
        if (
            (value > MAX_CAPTURE_WIDTH) ||
            (value < MIN_CAPTURE_WIDTH)
        ){
            return false;
        }

        FRAME_BUFFER.r.w = value;
        push_capture_event(capture_event_e::new_video_mode);
    }
    else if (key == "height")
    {
        if (
            (value > MAX_CAPTURE_HEIGHT) ||
            (value < MIN_CAPTURE_HEIGHT)
        ){
            return false;
        }

        FRAME_BUFFER.r.h = value;
        push_capture_event(capture_event_e::new_video_mode);
    }
    else if (key == "input channel index")
    {
        kc_ev_input_channel_changed.fire(value);
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

capture_event_e kc_pop_event_queue(void)
{
    static const auto pop_capture_event = [](const capture_event_e event)
    {
        const bool oldFlagValue = CAPTURE_EVENT_FLAGS[(int)event];
        CAPTURE_EVENT_FLAGS[(int)event] = false;
        return oldFlagValue;
    };

    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        kc_ev_invalid_signal.fire();
        return capture_event_e::invalid_signal;
    }

    if (pop_capture_event(capture_event_e::new_video_mode))
    {
        kc_ev_new_proposed_video_mode.fire({resolution_s::from_capture_device()});
        return capture_event_e::new_video_mode;
    }

    if (pop_capture_event(capture_event_e::new_frame))
    {
        kc_ev_new_captured_frame.fire(FRAME_BUFFER);
        return capture_event_e::new_frame;
    }

    return capture_event_e::none;
}

bool kc_release_device(void)
{
    delete [] FRAME_BUFFER.pixels;

    return true;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return FRAME_BUFFER;
}

uint kc_dropped_frames_count(void)
{
    return 0;
}
