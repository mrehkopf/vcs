/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#if !defined(CAPTURE_BACKEND_VIRTUAL) &&\
    !defined(CAPTURE_BACKEND_VISION_V4L) &&\
    !defined(CAPTURE_BACKEND_GENERIC_V4L) &&\
    !defined(CAPTURE_BACKEND_MMAP)
    #error "Unrecognized value for the capture backend toggle"
#endif

#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <deque>
#include "common/command_line/command_line.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "filter/filter.h"
#include "capture/video_presets.h"
#include "capture/alias.h"
#include "common/disk/disk.h"
#include "common/timer/timer.h"
#include "main.h"

#ifdef __SANITIZE_ADDRESS__
    #include <sanitizer/lsan_interface.h>
    const char* __lsan_default_options()
    {
        return (
            // Prevent __lsan_do_leak_check() from aborting when leaks are found.
            "exitcode=0:"

            // File located in relation to the program executable that optionally
            // suppresses leak reports for certain sources. For more info, see
            // https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppressions.
            "suppressions=asan-suppress.txt:"
        );
    }
#endif

// Callback funtions to be run in the main VCS loop after the loop's lock of the
// capture mutex has been released.
static std::queue<std::function<void(void)>> POST_MUTEX_CALLBACKS;

// Each subsystem of VCS may provide a release function that cleans up the
// subsystem, akin to a class destructor. This queue holds the release
// function for each initialized subsystem that provided one.
static std::deque<subsystem_releaser_t> SUBSYSTEM_RELEASERS;

// Set to true when we want to break out of the program's main loop and terminate.
/// TODO. Don't have this be global. Instead provide a function to request state changes on it.
bool PROGRAM_EXIT_REQUESTED = false;

static bool IS_ECO_MODE_ENABLED = false;
static auto ECO_REFERENCE_TIME = std::chrono::system_clock::now();

static void prepare_for_exit(void)
{
    INFO(("Received orders to exit. Initiating cleanup."));

    while (!SUBSYSTEM_RELEASERS.empty())
    {
        auto releaser_fn = SUBSYSTEM_RELEASERS.back();
        SUBSYSTEM_RELEASERS.pop_back();
        releaser_fn();
    }

    // Force the memory leak check to run now rather than on program exit, so that
    // it ignores allocations that remain on exit and which we let the OS clean up.
    #ifdef __SANITIZE_ADDRESS__
      __lsan_do_leak_check();
    #endif

    INFO(("Ready to exit."));

    return;
}

static bool initialize_all(void)
{
    // Listen for app events.
    {
        ev_eco_mode_enabled.listen([]
        {
            ECO_REFERENCE_TIME = std::chrono::system_clock::now();
        });

        ev_new_video_mode.listen([](const video_mode_s &videoMode)
        {
            INFO((
                "Video mode: %u x %u at %.3f Hz.",
                videoMode.resolution.w,
                videoMode.resolution.h,
                videoMode.refreshRate.value<double>()
            ));
        });

        // The capture device has received a new video mode. We'll inspect the
        // mode to see if we think it's acceptable, then allow news of it to
        // propagate to the rest of VCS.
        ev_new_proposed_video_mode.listen([](const video_mode_s &videoMode)
        {
            if (ka_has_alias(videoMode.resolution))
            {
                const resolution_s aliasedResolution = ka_aliased(videoMode.resolution);

                DEBUG((
                    "Aliasing %u x %u to %u x %u",
                     videoMode.resolution.w,
                     videoMode.resolution.h,
                     aliasedResolution.w,
                     aliasedResolution.h
                ));

                resolution_s::to_capture_device_properties(aliasedResolution);
            }
            else
            {
                ev_new_video_mode.fire(videoMode);
            }
        });
    }

    // Initialize subsystems.
    {
        SUBSYSTEM_RELEASERS.push_back(kvideopreset_initialize());
        SUBSYSTEM_RELEASERS.push_back(ks_initialize_scaler());
        SUBSYSTEM_RELEASERS.push_back(kc_initialize_capture());
        SUBSYSTEM_RELEASERS.push_back(kf_initialize_filters());

        // The display subsystem should be initialized last.
        SUBSYSTEM_RELEASERS.push_back(kd_acquire_output_window());
    }

    return !PROGRAM_EXIT_REQUESTED;
}

static capture_event_e handle_next_capture_event(void)
{
    const capture_event_e e = kc_process_next_capture_event();

    switch (e)
    {
        case capture_event_e::new_frame:
        {
            ev_frame_processing_finished.fire(kc_frame_buffer());
            break;
        }
        case capture_event_e::unrecoverable_error:
        {
            k_assert(0, "The capture device has reported an unrecoverable error.");
            break;
        }
        case capture_event_e::invalid_signal:
        {
            NBENE(("The input signal is out of range."));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            break;
        }
        case capture_event_e::invalid_device:
        {
            NBENE(("Invalid capture device."));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            break;
        }
        default:
        {
            break;
        }
    }

    return e;
}

// Load in any data files that the user requested via the command-line.
static void load_user_data(void)
{
    kd_load_video_presets(kcom_video_presets_file_name());
    kd_load_filter_graph(kcom_filter_graph_file_name());

    return;
}

void k_defer_until_capture_mutex_unlocked(std::function<void(void)> callback)
{
    POST_MUTEX_CALLBACKS.push(callback);

    return;
}

void k_set_eco_mode_enabled(const bool isEnabled)
{
    if (isEnabled == IS_ECO_MODE_ENABLED)
    {
        return;
    }

    IS_ECO_MODE_ENABLED = isEnabled;
    isEnabled? ev_eco_mode_enabled.fire() : ev_eco_mode_disabled.fire();

    return;
}

bool k_is_eco_mode_enabled(void)
{
    return IS_ECO_MODE_ENABLED;
}

// VCS's eco mode. Tries to figure out the average latency between capture events and
// put this main VCS thread to sleep for nearly that long between the capture events,
// rather than having the CPU spin its wheels in a busy loop for that duration.
//
// There's basically two categories in VCS capture events: something happened, or nothing
// happened. In a non-eco busy loop, the main VCS thread will keep polling the capture
// thread for new events, mostly receiving the 'nothing happened' event until a new frame
// comes in. This wastes CPU cycles but ensures low latency in reacting to capture events.
// In eco mode, the idea is that capture events generally occur at predictable intervals,
// so if one is happening e.g. every ~16 milliseconds (new captured frame at 60 Hz) and
// it's taking VCS ~4 ms to process each event, we can reasonably assume that we can sleep
// for at least 8 ms without missing the next one. So in that case we sleep for 8 ms (as
// an example), then return to the busy loop to be ready to immediately receive the next
// frame when it comes in.
static void eco_sleep(const capture_event_e event)
{
    if (!IS_ECO_MODE_ENABLED)
    {
        return;
    }

    const double maxTimeToSleepMs = 20;

    if (!kc_has_signal())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(unsigned(maxTimeToSleepMs)));
        return;
    }
    else
    {
        static double timeToSleepMs = 0;
        static unsigned numDroppedFrames = kc_dropped_frames_count();
        const double msSinceLastEvent = (0.85 * std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ECO_REFERENCE_TIME).count());

        const unsigned numNewDroppedFrames = (kc_dropped_frames_count() - numDroppedFrames);
        numDroppedFrames += numNewDroppedFrames;

        // If we drop frames, we shorten the sleep duration, and otherwise we creep toward
        // the maximum possible sleep time given the current rate of capture events.
        timeToSleepMs = std::lerp((timeToSleepMs / (numNewDroppedFrames? 1.5 : 1)), msSinceLastEvent, 0.01);
        timeToSleepMs = std::min(maxTimeToSleepMs, timeToSleepMs);

        // We have time to sleep only if we're not dropping frames.
        if (!numNewDroppedFrames)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(unsigned(timeToSleepMs)));
        }
    }

    if (event != capture_event_e::sleep)
    {
        ECO_REFERENCE_TIME = std::chrono::system_clock::now();
    }

    return;
}

int main(int argc, char *argv[])
{
    printf("VCS %u.%u.%u\n", VCS_VERSION.major, VCS_VERSION.minor, VCS_VERSION.patch);
#ifdef VCS_DEBUG_BUILD
    printf("NON-RELEASE BUILD\n");
#endif
    printf(":::::::::\n");

    INFO(("Initializing VCS."));
    try
    {
        if (!kcom_parse_command_line(argc, argv))
        {
            NBENE(("Malformed command line argument(s). Exiting."));
            goto fail;
        }

        if (!initialize_all())
        {
            kd_show_headless_error_message(
                "",
               "VCS has to exit because it encountered one or more unrecoverable errors "
               "while initializing itself. More information will have been printed into "
               "the console. If a console window was not already open, run VCS again "
               "from the command line."
            );
            goto fail;
        }

        if (!kc_has_signal())
        {
            ev_capture_signal_lost.fire();
        }

        load_user_data();
    }
    // Generally assumed to be from k_assert(), which will already have displayed
    // a more detailed error report to the user.
    catch (...)
    {
        NBENE(("VCS has encountered an error while initializing and will attempt to exit normally."));
        PROGRAM_EXIT_REQUESTED = true;
        goto fail;
    }

    DEBUG(("Entering the main loop."));
    try
    {
        while (!PROGRAM_EXIT_REQUESTED)
        {
            capture_event_e e;
            std::chrono::time_point<std::chrono::steady_clock> frameTimestamp;

            {
                LOCK_CAPTURE_MUTEX_IN_SCOPE;
                e = handle_next_capture_event();
                frameTimestamp = kc_frame_buffer().timestamp;
                kt_update_timers();
                kd_spin_event_loop();
            }

            while (!POST_MUTEX_CALLBACKS.empty())
            {
                POST_MUTEX_CALLBACKS.front()();
                POST_MUTEX_CALLBACKS.pop();
            }

            if (e == capture_event_e::new_frame)
            {
                ev_capture_processing_latency.fire(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - frameTimestamp
                    ).count()
                );
            }

            eco_sleep(e);
        }
    }
    // Generally assumed to be from k_assert(), which will already have displayed
    // a more detailed error report to the user.
    catch (...)
    {
        NBENE(("VCS has encountered a run-time error and will attempt to exit normally."));
        PROGRAM_EXIT_REQUESTED = true;
        goto fail;
    }

    prepare_for_exit();
    return EXIT_SUCCESS;

    fail:
    prepare_for_exit();
    return EXIT_FAILURE;
}
