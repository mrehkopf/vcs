/*
 * 2024 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <future>
#include <chrono>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <gphoto2/gphoto2.h>
#include "common/timer/timer.h"
#include "capture/capture.h"

static std::unordered_map<std::string, intptr_t> DEVICE_PROPERTIES = {
    {"api name", intptr_t("gPhoto2")},
    {"has signal", false},
    {"supports live preview", false},
    {"supports taking photo", false},
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},
    {"height: maximum", MAX_CAPTURE_HEIGHT},
};

static Camera *CAMERA;
static GPContext *GP_CONTEXT;

static unsigned NUM_FRAMES_PER_SECOND = 0;
static captured_frame_s LOCAL_FRAME_BUFFER = {.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]()};
static std::vector<unsigned> CAPTURE_FLAGS(static_cast<unsigned>(capture_event_e::num_enumerators));

static std::future<void> LIVE_PREVIEW_THREAD;
static std::atomic<bool> RUN_LIVE_PREVIEW_LOOP = false;

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

static std::string get_config_value(const std::string &name)
{
    CameraWidget *rootWidget = nullptr;
    CameraWidget *childWidget = nullptr;
    const char *value = nullptr;

    /// TODO: Don't assume all values are strings.
    if (
        (GP_OK <= gp_camera_get_config(CAMERA, &rootWidget, GP_CONTEXT)) &&
        (GP_OK <= gp_widget_get_child_by_name(rootWidget, name.c_str(), &childWidget)) &&
        (GP_OK <= gp_widget_get_value(childWidget, &value))
    ){
        return (value? value : "unknown");
    }
    else
    {
        return "error";
    }
}

static bool set_config_value(const std::string &name, const std::string &value)
{
    CameraWidget *rootWidget = nullptr;
    CameraWidget *childWidget = nullptr;

    return (
        (GP_OK <= gp_camera_get_config(CAMERA, &rootWidget, GP_CONTEXT)) &&
        (GP_OK <= gp_widget_get_child_by_name(rootWidget, name.c_str(), &childWidget)) &&
        (GP_OK <= gp_widget_set_value(childWidget, value.c_str())) &&
        (GP_OK <= gp_camera_set_config(CAMERA, rootWidget, GP_CONTEXT))
    );
}

static bool does_camera_support_feature(int featureEnum)
{
    CameraAbilities abilities = {0};
    gp_camera_get_abilities(CAMERA, &abilities);
    return bool(abilities.operations & featureEnum);
}

static void release_camera(void)
{
    if (CAMERA)
    {
        gp_camera_exit(CAMERA, GP_CONTEXT);
        gp_camera_unref(CAMERA);
        CAMERA = nullptr;
    }

    kc_set_device_property("supports live preview", false);
    kc_set_device_property("supports taking photo", false);
}

static void initialize_camera(void)
{
    if (CAMERA)
    {
        return;
    }

    gp_camera_new(&CAMERA);

    if (GP_OK > gp_camera_init(CAMERA, GP_CONTEXT))
    {
        release_camera();
        return;
    }

    kc_set_device_property("supports live preview", does_camera_support_feature(GP_OPERATION_CAPTURE_PREVIEW));
    kc_set_device_property("supports taking photo", does_camera_support_feature(GP_OPERATION_CAPTURE_IMAGE));
}

static void capture_loop(void)
{
    CameraFile *file = nullptr;
    const char *rawImageData = nullptr;
    unsigned long rawImageDataLen = 0;

    gp_file_new(&file);

    while (RUN_LIVE_PREVIEW_LOOP)
    {
        switch (gp_camera_capture_preview(CAMERA, file, GP_CONTEXT))
        {
            case GP_OK: break;
            default: continue;
        }

        LOCK_CAPTURE_MUTEX_IN_SCOPE;
        kc_set_device_property("has signal", true);

        // An error of some kind occurred.
        if (GP_OK > gp_file_get_data_and_size(file, &rawImageData, &rawImageDataLen))
        {
            kc_set_device_property("has signal", false);
            continue;
        }

        const cv::Mat image = cv::imdecode(cv::Mat(1, rawImageDataLen, CV_8UC1, (char*)rawImageData), cv::IMREAD_COLOR);

        if (!image.data)
        {
            kc_set_device_property("has signal", false);
            continue;
        }

        if (
            (kc_device_property("width") != image.cols) ||
            (kc_device_property("height") != image.rows)
        ){
            push_event(capture_event_e::new_video_mode);
        }

        kc_set_device_property("width", image.cols);
        kc_set_device_property("height", image.rows);

        push_event(capture_event_e::new_frame);

        for (unsigned y = 0; y < image.rows; y++)
        {
            for (unsigned x = 0; x < image.cols; x++)
            {
                const unsigned srcIdx = (((x % image.cols) + (y % image.rows) * image.cols) * 3);
                const unsigned dstIdx = ((x + y * image.cols) * 4);

                for (unsigned c = 0; c < 3; c++)
                {
                    LOCAL_FRAME_BUFFER.pixels[dstIdx+c] = image.data[srcIdx+c];
                }
            }
        }

        LOCAL_FRAME_BUFFER.timestamp = std::chrono::steady_clock::now();
    }
}

static void start_live_preview(void)
{
    RUN_LIVE_PREVIEW_LOOP = true;
    LIVE_PREVIEW_THREAD = std::async(std::launch::async, capture_loop);
}

static void stop_live_preview(void)
{
    if (!RUN_LIVE_PREVIEW_LOOP)
    {
        return;
    }

    RUN_LIVE_PREVIEW_LOOP = false;

    if (LIVE_PREVIEW_THREAD.valid())
    {
        LIVE_PREVIEW_THREAD.wait();
    }

    gp_camera_exit(CAMERA, GP_CONTEXT);
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the GPhoto capture device."));

    GP_CONTEXT = gp_context_new();
    initialize_camera();

    kc_set_device_property("width", 640);
    kc_set_device_property("height", 480);

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        refresh_rate_s::to_capture_device_properties((NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs)));
        NUM_FRAMES_PER_SECOND = 0;
    });

    ev_new_captured_frame.listen([]
    {
        NUM_FRAMES_PER_SECOND++;
    });

    // Create some custom GUI entries in VCS's control panel.
    {
        // Camera.
        {
            const auto camera_name = []()->std::string
            {
                CameraAbilities abilities;

                if (GP_OK <= gp_camera_get_abilities(CAMERA, &abilities))
                {
                    return abilities.model;
                }
                else
                {
                    return "⚠ No supported camera found.";
                }
            };

            auto *selectedCamera = new abstract_gui_widget::label;
            selectedCamera->text = camera_name();

            auto *refresh = new abstract_gui_widget::button;
            refresh->label = "Refresh";
            refresh->on_press = [selectedCamera, camera_name]()
            {
                release_camera();
                initialize_camera();
                selectedCamera->set_text(camera_name());
            };

            static abstract_gui_s gui;
            gui.layout = abstract_gui_s::layout_e::vertical_box;
            gui.fields.push_back({"", {selectedCamera}});
            gui.fields.push_back({"", {refresh}});
            kd_add_control_panel_widget("Capture", "Camera", &gui);
        }

        // Actions.
        {
            auto *livePreviewEnabled = new abstract_gui_widget::combo_box;
            livePreviewEnabled->items = {"Off", "On"};
            livePreviewEnabled->on_change = [](int idx){kc_set_device_property("live view enabled", idx);};
            livePreviewEnabled->initialIndex = 0;
            livePreviewEnabled->isInitiallyEnabled = CAMERA;

            auto *takePhoto = new abstract_gui_widget::button;
            takePhoto->label = "Take";
            takePhoto->isInitiallyEnabled = CAMERA;
            takePhoto->on_press = []()
            {
                if (!CAMERA)
                {
                    DEBUG(("Attempting to actuate the shutter while there is no camera. Ignoring this."));
                    return;
                }

                k_defer_until_capture_mutex_unlocked([]
                {
                    const bool wasLivePreviewEnabled = kc_device_property("live view enabled");

                    if (wasLivePreviewEnabled)
                    {
                        stop_live_preview();
                    }

                    CameraFilePath path = {0};
                    gp_camera_capture(CAMERA, GP_CAPTURE_IMAGE, &path, GP_CONTEXT);
                    DEBUG(("Captured an image to %s/%s", path.folder, path.name));

                    if (wasLivePreviewEnabled)
                    {
                        start_live_preview();
                    }
                });
            };

            static abstract_gui_s gui;
            gui.fields.push_back({"Photo", {takePhoto}});
            gui.fields.push_back({"Preview", {livePreviewEnabled}});
            kd_add_control_panel_widget("Capture", "Actions", &gui);

            kt_timer(1000, [=](const unsigned)
            {
                livePreviewEnabled->set_enabled(kc_device_property("supports live preview"));
                takePhoto->set_enabled(kc_device_property("supports taking photo"));
            });
        }

        // Options.
        {
            static std::string shutterSpeedValue = "1/x";
            auto *shutterSpeed = new abstract_gui_widget::line_edit;
            shutterSpeed->text = shutterSpeedValue;
            shutterSpeed->isInitiallyEnabled = CAMERA;
            shutterSpeed->on_change = [](const std::string &text)
            {
                shutterSpeedValue = text;
            };

            auto *getShutterSpeed = new abstract_gui_widget::button;
            getShutterSpeed->label = "Get current";
            getShutterSpeed->isInitiallyEnabled = CAMERA;
            getShutterSpeed->on_press = [shutterSpeed]()
            {
                if (!CAMERA)
                {
                    DEBUG(("Attempting to get the shutter speed while there is no camera. Ignoring this."));
                    return;
                }

                shutterSpeed->set_text(get_config_value("shutterspeed"));
            };

            auto *setShutterSpeed = new abstract_gui_widget::button;
            setShutterSpeed->label = "Set";
            setShutterSpeed->isInitiallyEnabled = CAMERA;
            setShutterSpeed->on_press = [shutterSpeed]()
            {
                if (!CAMERA)
                {
                    DEBUG(("Attempting to set the shutter speed while there is no camera. Ignoring this."));
                    return;
                }

                if (!set_config_value("shutterspeed", shutterSpeedValue))
                {
                    NBENE(("Failed to configure the shutter speed."));
                }

                DEBUG(("The shutter speed is now %s", get_config_value("shutterspeed").c_str()));
                shutterSpeed->set_text(get_config_value("shutterspeed"));
            };

            static std::string isoValue = "1";
            auto *iso = new abstract_gui_widget::line_edit;
            iso->text = isoValue;
            iso->isInitiallyEnabled = CAMERA;
            iso->on_change = [](const std::string &text)
            {
                isoValue = text;
            };

            auto *getIso= new abstract_gui_widget::button;
            getIso->label = "Get current";
            getIso->isInitiallyEnabled = CAMERA;
            getIso->on_press = [iso]()
            {
                if (!CAMERA)
                {
                    DEBUG(("Attempting to get the ISO value while there is no camera. Ignoring this."));
                    return;
                }

                iso->set_text(get_config_value("iso"));
            };

            auto *setIso = new abstract_gui_widget::button;
            setIso->label = "Set";
            setIso->isInitiallyEnabled = CAMERA;
            setIso->on_press = [iso]()
            {
                if (!CAMERA)
                {
                    DEBUG(("Attempting to set the ISO value while there is no camera. Ignoring this."));
                    return;
                }

                if (!set_config_value("iso", isoValue))
                {
                    NBENE(("Failed to configure the ISO."));
                }

                DEBUG(("The ISO is now %s", get_config_value("iso").c_str()));
                iso->set_text(get_config_value("iso"));
            };

            static abstract_gui_s gui;
            gui.fields.push_back({"Shutter", {shutterSpeed, setShutterSpeed, getShutterSpeed}});
            gui.fields.push_back({"ISO", {iso, setIso, getIso}});
            kd_add_control_panel_widget("Capture", "Options", &gui);

            kt_timer(1000, [=](const unsigned)
            {
                iso->set_enabled(CAMERA);
                setIso->set_enabled(CAMERA);
                getIso->set_enabled(CAMERA);

                shutterSpeed->set_enabled(CAMERA);
                setShutterSpeed->set_enabled(CAMERA);
                getShutterSpeed->set_enabled(CAMERA);
            });
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
        LOCAL_FRAME_BUFFER.resolution.w = value;
    }
    else if (key == "height")
    {
        LOCAL_FRAME_BUFFER.resolution.h = value;
    }
    else if (key == "live view enabled")
    {
        k_defer_until_capture_mutex_unlocked([value]
        {
            if (value)
            {
                start_live_preview();
                kc_set_device_property("has signal", true);
            }
            else
            {
                stop_live_preview();
                kc_set_device_property("has signal", false);
            }
        });
    }
    else if (
        (key == "has signal") &&
        (DEVICE_PROPERTIES[key] != value)
    ){
        push_event(value? capture_event_e::signal_gained : capture_event_e::signal_lost);
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

    if (!CAMERA)
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
    stop_live_preview();
    gp_context_unref(GP_CONTEXT);
    return true;
}
