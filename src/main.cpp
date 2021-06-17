/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS main
 *
 */

#include <chrono>
#include <thread>
#include <mutex>
#include "display/qt/windows/output_window.h"
#include "common/command_line/command_line.h"
#include "anti_tear/anti_tear.h"
#include "common/propagate/app_events.h"
#include "capture/capture_device.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "capture/alias.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "filter/filter.h"
#include "capture/video_presets.h"
#include "common/memory/memory.h"
#include "common/disk/disk.h"
#include "common/timer/timer.h"

// Set to !0 when we want to exit the program.
/// TODO. Don't have this global.
i32 PROGRAM_EXIT_REQUESTED = 0;

static void cleanup_all(void)
{
    INFO(("Received orders to exit. Initiating cleanup."));

    kt_release_timers();
    if (krecord_is_recording())
    {
        krecord_stop_recording();
    }
    kd_release_output_window();
    ks_release_scaler();
    kc_release_capture();
    kat_release_anti_tear();
    kf_release_filters();
    kvideopreset_release();
    krecord_release();

    // Call this last.
    kmem_deallocate_memory_cache();

    INFO(("Ready to exit. Bye!"));
    return;
}

static bool initialize_all(void)
{
    ke_events().capture.unrecoverableError.subscribe([]
    {
        PROGRAM_EXIT_REQUESTED = true;
    });

    ke_events().capture.newVideoMode.subscribe([]
    {
        const auto resolution = kc_capture_device().get_resolution();

        INFO(("Video mode: %u x %u @ %.3f Hz.", resolution.w, resolution.h, kc_capture_device().get_refresh_rate().value<double>()));
    });

    // The capture device has received a new video mode. We'll inspect the
    // mode to see if we think it's acceptable, then allow news of it to
    // propagate to the rest of VCS.
    ke_events().capture.newProposedVideoMode.subscribe([]
    {
        const auto resolution = kc_capture_device().get_resolution();

        // If there's an alias for this resolution, force that resolution
        // instead. Note that forcing the resolution is expected to automatically
        // fire a capture.newVideoMode event.
        if (ka_has_alias(resolution))
        {
            const resolution_s aliasResolution = ka_aliased(resolution);

            kc_force_input_resolution(aliasResolution);
        }
        else
        {
            ke_events().capture.newVideoMode.fire();
        }
    });

    if (!PROGRAM_EXIT_REQUESTED) kt_initialize_timers();
    if (!PROGRAM_EXIT_REQUESTED) ka_initialize_aliases();
    if (!PROGRAM_EXIT_REQUESTED) krecord_initialize();
    if (!PROGRAM_EXIT_REQUESTED) klog_initialize();
    if (!PROGRAM_EXIT_REQUESTED) kvideopreset_initialize();
    if (!PROGRAM_EXIT_REQUESTED) ks_initialize_scaler();
    if (!PROGRAM_EXIT_REQUESTED) kc_initialize_capture();
    if (!PROGRAM_EXIT_REQUESTED) kat_initialize_anti_tear();
    if (!PROGRAM_EXIT_REQUESTED) kf_initialize_filters();

    // Ideally, do these last.
    if (!PROGRAM_EXIT_REQUESTED)
    {
        kd_acquire_output_window();
    }

    return !PROGRAM_EXIT_REQUESTED;
}

static capture_event_e process_next_capture_event(void)
{
    std::lock_guard<std::mutex> lock(kc_capture_device().captureMutex);

    const capture_event_e e = kc_capture_device().pop_capture_event_queue();

    switch (e)
    {
        case capture_event_e::unrecoverable_error:
        {
            NBENE(("The capture device has reported an unrecoverable error."));

            ke_events().capture.unrecoverableError.fire();
            
            break;
        }
        case capture_event_e::new_frame:
        {
            if (kc_capture_device().has_valid_signal())
            {
                ke_events().capture.newFrame.fire();
            }
            else
            {
                kc_capture_device().mark_frame_buffer_as_processed();
            }
            
            break;
        }
        case capture_event_e::new_video_mode:
        {
            if (kc_capture_device().has_valid_signal())
            {
                ke_events().capture.newProposedVideoMode.fire();
            }

            break;
        }
        case capture_event_e::signal_lost:
        {
            ke_events().capture.signalLost.fire();
            break;
        }
        case capture_event_e::invalid_signal:
        {
            ke_events().capture.invalidSignal.fire();
            break;
        }
        case capture_event_e::invalid_device:
        {
            ke_events().capture.invalidDevice.fire();
            break;
        }
        case capture_event_e::sleep:
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(4)); /// TODO. Is 4 the best wait-time?

            break;
        }
        case capture_event_e::none:
        {
            /// TODO. Technically we might loop until we get an event other than
            /// none, but for now we just move on.

            break;
        }
        default:
        {
            k_assert(0, "Unhandled capture event.");
        }
    }

    return e;
}

// Load in any data files that the user requested via the command-line.
static void load_user_data(void)
{
    kd_load_video_presets(kcom_video_presets_file_name());
    kd_load_filter_graph(kcom_filter_graph_file_name());
    kd_load_aliases(kcom_aliases_file_name());

    return;
}

int main(int argc, char *argv[])
{
    printf("VCS %s\n---------+\n", PROGRAM_VERSION_STRING);

    #ifndef RELEASE_BUILD
        printf("NON-RELEASE BUILD\n");
    #endif

    // We want to be sure that the capture hardware is released in a controlled
    // manner, if possible, in the event of a runtime failure. (Not releasing the
    // hardware on program termination may cause a degradation in its subsequent
    // performance until the computer is rebooted.)
    std::set_terminate([]
    {
        NBENE(("VCS has encountered a runtime error and has decided that it's best "
               "to close down."));

        PROGRAM_EXIT_REQUESTED = true;
        cleanup_all();
    });

    INFO(("Parsing the command line."));
    if (!kcom_parse_command_line(argc, argv))
    {
        NBENE(("Command line parse failed. Exiting."));
        return 1;
    }

    INFO(("Initializing the program."));
    if (!initialize_all())
    {
        kd_show_headless_error_message("",
                                       "VCS has to exit because it encountered one or more "
                                       "unrecoverable errors while initializing itself. "
                                       "More information will have been printed into "
                                       "the console. If a console window was not already "
                                       "open, run VCS again from the command line.");

        cleanup_all();
        return EXIT_FAILURE;
    }
    else
    {
        load_user_data();
    }

    INFO(("Entering the main loop."));
    {
        // Propagate the initial video mode to VCS.
        if (kc_capture_device().has_valid_signal())
        {
            ke_events().capture.newProposedVideoMode.fire();
        }

        while (!PROGRAM_EXIT_REQUESTED)
        {
            process_next_capture_event();
            kt_update_timers();
            kd_spin_event_loop();
        }
    }

    cleanup_all();
    return EXIT_SUCCESS;
}
