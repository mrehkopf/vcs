/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * A sample implementation of a basic capture backend for VCS, targeting a generic
 * Vision4Linux-compatible capture device.
 *
 * This implementation is intended for educational purposes only.
 *
 */

#include <future>
#include <chrono>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "common/timer/timer.h"
#include "capture/capture.h"

static unsigned NUM_FRAMES_PER_SECOND = 0;
static captured_frame_s LOCAL_FRAME_BUFFER = {.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]()};
static std::vector<unsigned> CAPTURE_FLAGS(static_cast<unsigned>(capture_event_e::num_enumerators));

static std::future<void> CAPTURE_THREAD;
static cv::VideoCapture CAPTURE_DEVICE;
static cv::Mat DEVICE_FRAME_BUFFER;

static std::atomic<bool> RUN_CAPTURE_LOOP = false;
static bool RESET_RESOLUTION = false;

// These properties will appear in the "Video presets" section of VCS's control panel.
static std::vector<const char*> SUPPORTED_VIDEO_PROPERTIES = {
    "Brightness",
    "Focus",
    "Zoom",
};

static std::unordered_map<std::string, intptr_t> DEVICE_PROPERTIES = {
    {"api name", intptr_t("Video4Linux")},
    {"channel", 0},
    {"autofocus", 0},
    {"fps", 60},
    {"has signal", true},
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},
    {"height: maximum", MAX_CAPTURE_HEIGHT},
    {"supported video preset properties", intptr_t(&SUPPORTED_VIDEO_PROPERTIES)},
    {"supports video presets", true},
    {"supports channel switching", true},
    {"supports resolution switching", true},
};

static void push_event(capture_event_e flag)
{
    CAPTURE_FLAGS[int(flag)] = true;
}

static bool pop_event(const capture_event_e flag)
{
    const bool flagValue = CAPTURE_FLAGS[int(flag)];
    CAPTURE_FLAGS[int(flag)] = false;
    return flagValue;
}

static void capture_loop(void)
{
    while (RUN_CAPTURE_LOOP)
    {
        if (
            CAPTURE_DEVICE.isOpened() &&
            CAPTURE_DEVICE.read(DEVICE_FRAME_BUFFER) &&
            !DEVICE_FRAME_BUFFER.empty()
        ){
            cv::cvtColor(DEVICE_FRAME_BUFFER, DEVICE_FRAME_BUFFER, cv::COLOR_RGB2RGBA);

            LOCK_CAPTURE_MUTEX_IN_SCOPE;
            push_event(capture_event_e::new_frame);
            LOCAL_FRAME_BUFFER.timestamp = std::chrono::steady_clock::now();
            std::memcpy(
                LOCAL_FRAME_BUFFER.pixels,
                DEVICE_FRAME_BUFFER.data,
                std::min(MAX_NUM_BYTES_IN_CAPTURED_FRAME, (DEVICE_FRAME_BUFFER.total() * DEVICE_FRAME_BUFFER.elemSize()))
            );
        }
    }
}

static void release_capture_device(void)
{
    RUN_CAPTURE_LOOP = false;
    CAPTURE_THREAD.wait();
    CAPTURE_DEVICE.release();
}

static void acquire_capture_device(void)
{
    kc_set_device_property(
        "has signal",
        CAPTURE_DEVICE.open(kc_device_property("channel"), cv::CAP_V4L2) &&
        CAPTURE_DEVICE.isOpened()
    );
}

static void start_capture(void)
{
    // Persist certain settings between capture sessions.
    CAPTURE_DEVICE.set(cv::CAP_PROP_BUFFERSIZE, kc_device_property("buffer size"));
    CAPTURE_DEVICE.set(cv::CAP_PROP_FPS, kc_device_property("fps"));
    CAPTURE_DEVICE.set(cv::CAP_PROP_AUTOFOCUS, kc_device_property("autofocus"));

    RUN_CAPTURE_LOOP = true;
    CAPTURE_THREAD = std::async(std::launch::async, capture_loop);
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the generic V4L capture device."));

    acquire_capture_device();

    kc_set_device_property("buffer size", 1);
    kc_set_device_property("width", CAPTURE_DEVICE.get(cv::CAP_PROP_FRAME_WIDTH));
    kc_set_device_property("height", CAPTURE_DEVICE.get(cv::CAP_PROP_FRAME_HEIGHT));
    kc_set_device_property("Brightness: minimum", 0);
    kc_set_device_property("Brightness: maximum", 255);
    kc_set_device_property("Brightness: default", 63);
    kc_set_device_property("Brightness", 63);
    kc_set_device_property("Focus: minimum", 0);
    kc_set_device_property("Focus: maximum", 1023);
    kc_set_device_property("Focus: default", 127);
    kc_set_device_property("Focus", 127);
    kc_set_device_property("Zoom: minimum", 0);
    kc_set_device_property("Zoom: maximum", 1023);
    kc_set_device_property("Zoom: default", 127);
    kc_set_device_property("Zoom", 127);

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        refresh_rate_s::to_capture_device_properties((NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs)));
        NUM_FRAMES_PER_SECOND = 0;
    });

    ev_new_captured_frame.listen([]
    {
        NUM_FRAMES_PER_SECOND++;
    });

    ev_new_video_mode.listen([](const video_mode_s &mode)
    {
        capture_rate_s::to_capture_device_properties(mode.refreshRate);
    });

    // Create some custom GUI entries in VCS's control panel.
    {
        auto *fps = new abstract_gui_widget::combo_box;
        fps->items = {"60", "50", "30", "25", "10"};
        fps->on_change = [fps](int idx){kc_set_device_property("fps", std::stoi(fps->items[idx]));};
        fps->initialIndex = 0;

        auto *autoFocus = new abstract_gui_widget::combo_box;
        autoFocus->items = {"Off", "On"};
        autoFocus->on_change = [](int idx){kc_set_device_property("autofocus", idx);};
        autoFocus->initialIndex = 0;

        static abstract_gui_s gui;
        gui.fields.push_back({"Autofocus", {autoFocus}});
        gui.fields.push_back({"FPS", {fps}});
        kd_add_control_panel_widget("Capture", "Video4Linux control properties", &gui);
    }

    start_capture();

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
        LOCAL_FRAME_BUFFER.resolution.w = value;
        RESET_RESOLUTION = true;
    }
    else if (key == "height")
    {
        LOCAL_FRAME_BUFFER.resolution.h = value;
        RESET_RESOLUTION = true;
    }
    else if (key == "refresh rate")
    {
        if (value != kc_device_property("refresh rate"))
        {
            push_event(capture_event_e::new_video_mode);
        }
    }
    else if (key == "buffer size")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_BUFFERSIZE, value);
    }
    else if (key == "channel")
    {
        k_defer_until_capture_mutex_unlocked([]
        {
            // Note: By the time this code runs, the new channel value will have
            // been entered into the device properties, from which it'll be read
            // by acquire_capture_device().
            release_capture_device();
            acquire_capture_device();
            start_capture();
            ev_new_input_channel.fire(kc_device_property("channel"));
        });
    }
    else if (key == "fps")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_FPS, value);
    }
    else if (key == "autofocus")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_AUTOFOCUS, value);
    }
    else if (key == "Focus")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_FOCUS, value);
    }
    else if (key == "Zoom")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_ZOOM, value);
    }
    else if (key == "Brightness")
    {
        CAPTURE_DEVICE.set(cv::CAP_PROP_BRIGHTNESS, value);
    }
    else if (key == "has signal")
    {
        push_event(value? capture_event_e::signal_gained : capture_event_e::signal_lost);
    }

    if (RESET_RESOLUTION)
    {
        RESET_RESOLUTION = false;

        k_defer_until_capture_mutex_unlocked([]
        {
            // With a Datapath Vision capture card, changing the capture resolution
            // while capturing causes the capture to freeze, unless we first restart
            // the capture.
            release_capture_device();
            acquire_capture_device();

            CAPTURE_DEVICE.set(cv::CAP_PROP_FRAME_WIDTH, LOCAL_FRAME_BUFFER.resolution.w);
            CAPTURE_DEVICE.set(cv::CAP_PROP_FRAME_HEIGHT, LOCAL_FRAME_BUFFER.resolution.h);
            push_event(capture_event_e::new_video_mode);

            start_capture();
        });
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

capture_event_e kc_process_next_capture_event(void)
{
    if (pop_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }

    if (pop_event(capture_event_e::signal_gained))
    {
        ev_capture_signal_gained.fire();
        return capture_event_e::signal_gained;
    }

    if (pop_event(capture_event_e::signal_lost))
    {
        ev_capture_signal_lost.fire();
        return capture_event_e::signal_lost;
    }

    if (!CAPTURE_DEVICE.isOpened())
    {
        return capture_event_e::sleep;
    }

    if (pop_event(capture_event_e::new_video_mode))
    {
        ev_new_proposed_video_mode.fire(video_mode_s{
            .resolution = resolution_s::from_capture_device_properties(),
            .refreshRate = refresh_rate_s::from_capture_device_properties()
        });
        return capture_event_e::new_video_mode;
    }

    if (pop_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(LOCAL_FRAME_BUFFER);
        return capture_event_e::new_frame;
    }

    return capture_event_e::sleep;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return LOCAL_FRAME_BUFFER;
}

uint kc_dropped_frames_count(void)
{
    return 0;
}

bool kc_release_device(void)
{
    release_capture_device();
    return true;
}
