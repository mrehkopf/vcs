/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <unordered_map>
#include <chrono>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "common/globals.h"
#include "common/vcs_event/vcs_event.h"
#include "display/qt/persistent_settings.h"
#include "common/abstract_gui.h"
#include "common/timer/timer.h"
#include "capture/video_presets.h"
#include "capture/capture.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static captured_frame_s FRAME_BUFFER;

static cv::Mat BG_IMAGE;

enum class output_pattern_type : int
{
    animated = 0,
    non_animated = 1,
    image = 2,
} PATTERN_TYPE = output_pattern_type::animated;

static struct {
    abstract_gui_s patternGeneration;
} GUI;

static struct
{
    double brightness = 1;
} VIDEO_PARAMS;

static std::vector<const char*> SUPPORTED_VIDEO_PROPERTIES = {
    "Brightness",
};

static std::unordered_map<std::string, intptr_t> DEVICE_PROPERTIES = {
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"has signal", true},

    {"supported video preset properties", intptr_t(&SUPPORTED_VIDEO_PROPERTIES)},
    {"supports video presets", true},
    {"supports channel switching", true},
    {"supports resolution switching", true},
};

static void refresh_test_pattern(void)
{
    static unsigned numTicks = 0;

    if (PATTERN_TYPE == output_pattern_type::animated)
    {
        numTicks++;
    }

    FRAME_BUFFER.timestamp = std::chrono::steady_clock::now();

    if (
        (PATTERN_TYPE == output_pattern_type::image) &&
        (BG_IMAGE.data != nullptr)
    ){
        k_assert_optional(
            (BG_IMAGE.elemSize() == 3),
            "Expected the image to have 3 color channels."
        );

        for (unsigned y = 0; y < FRAME_BUFFER.resolution.h; y++)
        {
            for (unsigned x = 0; x < FRAME_BUFFER.resolution.w; x++)
            {
                const unsigned srcIdx = (((x % BG_IMAGE.cols) + (y % BG_IMAGE.rows) * BG_IMAGE.cols) * 3);
                const unsigned dstIdx = ((x + y * FRAME_BUFFER.resolution.w) * 4);

                for (unsigned c = 0; c < 3; c++)
                {
                    FRAME_BUFFER.pixels[dstIdx+c] = (BG_IMAGE.data[srcIdx+c] * VIDEO_PARAMS.brightness);
                }
            }
        }
    }
    else
    {
        for (unsigned y = 0; y < FRAME_BUFFER.resolution.h; y++)
        {
            for (unsigned x = 0; x < FRAME_BUFFER.resolution.w; x++)
            {
                const unsigned idx = ((x + y * FRAME_BUFFER.resolution.w) * 4);
                FRAME_BUFFER.pixels[idx + 0] = (150 * VIDEO_PARAMS.brightness);
                FRAME_BUFFER.pixels[idx + 1] = (((numTicks + y) % 256) * VIDEO_PARAMS.brightness);
                FRAME_BUFFER.pixels[idx + 2] = (((numTicks + x) % 256) * VIDEO_PARAMS.brightness);
                FRAME_BUFFER.pixels[idx + 3] = 255;
            }
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
        const auto inres = resolution_s::from_capture_device_properties();

        if ((inres.w > MAX_CAPTURE_WIDTH) || (inres.h > MAX_CAPTURE_HEIGHT))
        {
            push_capture_event(capture_event_e::invalid_signal);
        }
        else
        {
            push_capture_event(capture_event_e::new_frame);
            NUM_FRAMES_PER_SECOND++;
            refresh_test_pattern();
        }
    });

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        const unsigned newRefreshRate = (refresh_rate_s::numDecimalsPrecision * (NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs)));

        NUM_FRAMES_PER_SECOND = 0;

        if (kc_device_property("refresh rate") != newRefreshRate)
        {
            kc_set_device_property("refresh rate", newRefreshRate);
            push_capture_event(capture_event_e::new_video_mode);
        }
    });

    // Define analog control ranges.
    {
        analog_properties_s::to_capture_device_properties({
            {"Brightness", 0},
        }, ": minimum");

        analog_properties_s::to_capture_device_properties({
            {"Brightness", 63},
        }, ": maximum");

        analog_properties_s::to_capture_device_properties({
            {"Brightness", 63},
        }, ": default");

        analog_properties_s::to_capture_device_properties(
            analog_properties_s::from_capture_device_properties(": default")
        );
    }

    // Create custom GUI entries.
    {
        // "Pattern generation".
        {
            auto *const selectImage = new abstract_gui::button_get_open_filename;
            selectImage->label = "Select image...";
            selectImage->filenameFilter = "Images (*.png *.jpg *.bmp);;All files(*.*)";
            selectImage->isInitiallyEnabled = (PATTERN_TYPE == output_pattern_type::image);
            selectImage->on_success = [](const std::string &filename)
            {
                BG_IMAGE = cv::imread(filename);

                if (
                    (BG_IMAGE.data != nullptr) &&
                    (BG_IMAGE.elemSize() == 3) &&
                    (BG_IMAGE.cols <= int(MAX_CAPTURE_WIDTH)) &&
                    (BG_IMAGE.rows <= int(MAX_CAPTURE_HEIGHT)) &&
                    (BG_IMAGE.cols >= int(MIN_CAPTURE_WIDTH)) &&
                    (BG_IMAGE.rows >= int(MIN_CAPTURE_HEIGHT))
                ){
                    FRAME_BUFFER.resolution = {
                        .w = unsigned(BG_IMAGE.cols),
                        .h = unsigned(BG_IMAGE.rows)
                    };

                    kc_set_device_property("width", FRAME_BUFFER.resolution.w);
                    kc_set_device_property("height", FRAME_BUFFER.resolution.h);
                }
                else
                {
                    kd_show_headless_error_message(
                        "Image load error",
                        "The selected image is unsupported."
                    );
                }
            };

            auto *const patternType = new abstract_gui::combo_box;
            patternType->items = {"Animated", "Static", "Image from file"};
            patternType->initialIndex = unsigned(PATTERN_TYPE);
            patternType->on_change = [patternType, selectImage](const int itemsIdx)
            {
                PATTERN_TYPE = output_pattern_type(itemsIdx);
                kpers_set_value(INI_GROUP_CAPTURE, "VirtualPattern", itemsIdx);

                selectImage->set_enabled(PATTERN_TYPE == output_pattern_type::image);
            };

            GUI.patternGeneration.layout = abstract_gui_s::layout_e::vertical_box;
            GUI.patternGeneration.fields.push_back({"", {patternType}});
            GUI.patternGeneration.fields.push_back({"", {selectImage}});

            kd_add_control_panel_widget("Capture", "Pattern generation", &GUI.patternGeneration);
        }
    }

    return;
}

intptr_t kc_device_property(const std::string &key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string &key, intptr_t value)
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
    else if (key == "Brightness")
    {
        VIDEO_PARAMS.brightness = (value / double(kc_device_property("Brightness: maximum")));
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
            .resolution = resolution_s::from_capture_device_properties(),
            .refreshRate = refresh_rate_s::from_capture_device_properties()
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
