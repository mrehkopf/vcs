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
#include "filter/anti_tear.h"
#include "common/propagate/propagate.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "capture/alias.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "filter/filter.h"
#include "common/memory/memory.h"
#include "common/disk/disk.h"

// Set to !0 when we want to exit the program.
/// TODO. Don't have this global.
i32 PROGRAM_EXIT_REQUESTED = 0;

static void cleanup_all(void)
{
    DEBUG(("Received orders to exit. Initiating cleanup."));

    kd_release_output_window();
    ks_release_scaler();
    kc_release_capture();
    kat_release_anti_tear();
    kf_release_filters();

    if (krecord_is_recording()) krecord_stop_recording();

    // Call this last.
    kmem_deallocate_memory_cache();

    INFO(("Ready to exit."));
    return;
}

static bool initialize_all(void)
{
    if (!PROGRAM_EXIT_REQUESTED) ks_initialize_scaler();
    if (!PROGRAM_EXIT_REQUESTED) kc_initialize_capture();
    if (!PROGRAM_EXIT_REQUESTED) kat_initialize_anti_tear();
    if (!PROGRAM_EXIT_REQUESTED) kf_initialize_filters();

    // Ideally, do these last.
    if (!PROGRAM_EXIT_REQUESTED)
    {
        kd_acquire_output_window();
        ka_broadcast_aliases_to_gui();
    }

    return !PROGRAM_EXIT_REQUESTED;
}

static capture_event_e process_next_capture_event(void)
{
    std::lock_guard<std::mutex> lock(kc_capture_api().captureMutex);

    const capture_event_e e = kc_capture_api().pop_capture_event_queue();

    switch (e)
    {
        case capture_event_e::unrecoverable_error:
        {
            kpropagate_news_of_unrecoverable_error();
            break;
        }
        case capture_event_e::new_frame:
        {
            kpropagate_news_of_new_captured_frame();
            break;
        }
        case capture_event_e::new_video_mode:
        {
            kpropagate_news_of_new_capture_video_mode();
            break;
        }
        case capture_event_e::signal_lost:
        {
            kpropagate_news_of_lost_capture_signal();
            break;
        }
        case capture_event_e::invalid_signal:
        {
            kpropagate_news_of_invalid_capture_signal();
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
    kdisk_load_video_signal_parameters(kcom_params_file_name());
    kdisk_load_filter_graph(kcom_filters_file_name());
    kdisk_load_aliases(kcom_alias_file_name());

    return;
}

#ifndef VALIDATION_RUN
int main(int argc, char *argv[])
{
    printf("VCS %s\n---------+\n", PROGRAM_VERSION_STRING);

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
                                       "\n\nMore information will have been printed into "
                                       "the console. If a console window was not already "
                                       "open, run VCS again from the command line.");

        cleanup_all();
        return EXIT_FAILURE;
    }
    else
    {
        load_user_data();

        // Hack. Force the GUI to update itself on the current capture
        // parameters. This is needed if the user didn't ask for any
        // parameters to be loaded from disk, which would then call this
        // function automatically.
        kd_update_video_mode_params();
    }

    INFO(("Entering the main loop."));
    while (!PROGRAM_EXIT_REQUESTED)
    {
        process_next_capture_event();
        kd_spin_event_loop();
    }

    cleanup_all();
    return EXIT_SUCCESS;
}
#endif
