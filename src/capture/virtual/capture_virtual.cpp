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
#include "common/vcs_event/vcs_event.h"
#include "display/qt/persistent_settings.h"
#include "common/abstract_gui.h"
#include "common/timer/timer.h"
#include "capture/capture.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static captured_frame_s FRAME_BUFFER;

enum class output_pattern_type : int
{
    animated = 0,
    non_animated = 1,
} PATTERN_TYPE = output_pattern_type::animated;

static struct {
    abstract_gui_s patternGeneration;
} GUI;

static struct
{
    double brightness = 1;
    double redBrightness = 1;
    double greenBrightness = 1;
    double blueBrightness = 1;
} VIDEO_PARAMS;

static std::unordered_map<std::string, double> DEVICE_PROPERTIES = {
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},

    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"has signal", true},

    {"supports channel switching: ui", true},
    {"supports resolution switching: ui", true},
};

static void refresh_test_pattern(void)
{
    static unsigned numTicks = 0;
    numTicks++;

    for (unsigned y = 0; y < FRAME_BUFFER.resolution.h; y++)
    {
        for (unsigned x = 0; x < FRAME_BUFFER.resolution.w; x++)
        {
            QColor rgbGradient;
            const double widthRatio = (((x + numTicks) % FRAME_BUFFER.resolution.w) / double(FRAME_BUFFER.resolution.w));
            const double heightRatio = (y / double(FRAME_BUFFER.resolution.h));
            rgbGradient.setHsl((widthRatio * 359), 255, (255 - (heightRatio * 255)));
            rgbGradient = rgbGradient.toRgb().lighter(VIDEO_PARAMS.brightness * 100);

            const unsigned idx = ((x + y * FRAME_BUFFER.resolution.w) * 4);
            FRAME_BUFFER.pixels[idx + 0] = (rgbGradient.blue() * VIDEO_PARAMS.blueBrightness);
            FRAME_BUFFER.pixels[idx + 1] = (rgbGradient.green() * VIDEO_PARAMS.greenBrightness);
            FRAME_BUFFER.pixels[idx + 2] = (rgbGradient.red() * VIDEO_PARAMS.redBrightness);
            FRAME_BUFFER.pixels[idx + 3] = rgbGradient.alpha();
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
    FRAME_BUFFER.resolution = {.w = 640, .h = 480};
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    kc_set_device_property("channel", INPUT_CHANNEL_IDX);
    kc_set_device_property("width", FRAME_BUFFER.resolution.w);
    kc_set_device_property("height", FRAME_BUFFER.resolution.h);

    PATTERN_TYPE = output_pattern_type(kpers_value_of(INI_GROUP_CAPTURE, "VirtualPattern", 0).toInt());

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
            push_capture_event(capture_event_e::new_frame);
            NUM_FRAMES_PER_SECOND++;

            if (PATTERN_TYPE == output_pattern_type::animated)
            {
                refresh_test_pattern();
            }
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

    ev_new_video_mode.listen(refresh_test_pattern);

    video_signal_parameters_s::to_capture_device({
        .brightness         = 1,
        .contrast           = 1,
        .redBrightness      = 1,
        .redContrast        = 1,
        .greenBrightness    = 1,
        .greenContrast      = 1,
        .blueBrightness     = 1,
        .blueContrast       = 1,
        .horizontalSize     = 1,
        .horizontalPosition = 1,
        .verticalPosition   = 1,
        .phase              = 1,
        .blackLevel         = 1
    }, ": minimum");

    video_signal_parameters_s::to_capture_device({
        .brightness         = 200,
        .contrast           = 200,
        .redBrightness      = 200,
        .redContrast        = 200,
        .greenBrightness    = 200,
        .greenContrast      = 200,
        .blueBrightness     = 200,
        .blueContrast       = 200,
        .horizontalSize     = 200,
        .horizontalPosition = 200,
        .verticalPosition   = 200,
        .phase              = 200,
        .blackLevel         = 200
    }, ": maximum");

    video_signal_parameters_s::to_capture_device({
        .brightness         = 100,
        .contrast           = 100,
        .redBrightness      = 100,
        .redContrast        = 100,
        .greenBrightness    = 100,
        .greenContrast      = 100,
        .blueBrightness     = 100,
        .blueContrast       = 100,
        .horizontalSize     = 100,
        .horizontalPosition = 100,
        .verticalPosition   = 100,
        .phase              = 100,
        .blackLevel         = 100
    }, ": default");

    // Create custom GUI entries.
    {
        // "Pattern generation".
        {
            auto *const patternType = new abstract_gui::combo_box;
            patternType->items = {"Animated", "Static"};
            patternType->set_value = [patternType](const int itemsIdx){
                PATTERN_TYPE = output_pattern_type(itemsIdx);
                kpers_set_value(INI_GROUP_CAPTURE, "VirtualPattern", itemsIdx);
            };
            patternType->get_value = []{return int(PATTERN_TYPE);};

            GUI.patternGeneration.layout = abstract_gui_s::layout_e::vertical_box;
            GUI.patternGeneration.fields.push_back({"", {patternType}});

            kd_add_control_panel_widget("Capture", "Pattern generation", &GUI.patternGeneration);
        }
    }

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

        FRAME_BUFFER.resolution.w = value;
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

        FRAME_BUFFER.resolution.h = value;
        push_capture_event(capture_event_e::new_video_mode);
    }
    else if (key == "channel")
    {
        ev_new_input_channel.fire(value);
    }
    else if (key == "brightness")
    {
        VIDEO_PARAMS.brightness = (value / 100);
    }
    else if (key == "red brightness")
    {
        VIDEO_PARAMS.redBrightness = (value / 100);
    }
    else if (key == "green brightness")
    {
        VIDEO_PARAMS.greenBrightness = (value / 100);
    }
    else if (key == "blue brightness")
    {
        VIDEO_PARAMS.blueBrightness = (value / 100);
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

capture_event_e kc_process_next_capture_event(void)
{
    static const auto pop_capture_event = [](const capture_event_e event)
    {
        const bool oldFlagValue = CAPTURE_EVENT_FLAGS[(int)event];
        CAPTURE_EVENT_FLAGS[(int)event] = false;
        return oldFlagValue;
    };

    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        ev_invalid_capture_signal.fire();
        return capture_event_e::invalid_signal;
    }

    if (pop_capture_event(capture_event_e::new_video_mode))
    {
        ev_new_proposed_video_mode.fire(video_mode_s{
            .resolution = resolution_s::from_capture_device(),
            .refreshRate = refresh_rate_s::from_capture_device()
        });
        return capture_event_e::new_video_mode;
    }

    if (pop_capture_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(FRAME_BUFFER);
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
