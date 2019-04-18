/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS main
 *
 */

#include <chrono>
#include <thread>
#include <mutex>
#include "common/command_line.h"
#include "display/qt/d_window.h"
#include "filter/anti_tear.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "display/display.h"
#include "common/propagate.h"
#include "record/record.h"
#include "memory.h"
#include "scaler/scaler.h"
#include "common/globals.h"
#include "filter/filter.h"

extern std::mutex INPUT_OUTPUT_MUTEX;

// Set to !0 when we want to exit the program.
/// TODO. Don't have this global.
i32 PROGRAM_EXIT_REQUESTED = 0;

static void cleanup_all(void)
{
    INFO(("Received orders to exit. Initiating cleanup."));

    kd_release_output_window();
    ks_release_scaler();
    kc_release_capture();
    kat_release_anti_tear();
    kf_release_filters();

    if (krecord_is_recording()) krecord_stop_recording();

    // Call this last.
    kmem_deallocate_memory_cache();

    INFO(("Cleanup is done."));
    INFO(("Ready for exit."));
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
    std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

    #if USE_RGBEASY_API
        const capture_event_e e = kc_latest_capture_event();
    #else
        const capture_event_e e = capture_event_e::new_frame;
        kc_insert_test_image();
    #endif

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
        case capture_event_e::no_signal:
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
            k_assert(0, "Unrecognized capture event.");
        }
    }

    return e;
}

#ifndef VALIDATION_RUN
int main(int argc, char *argv[])
{
    printf("VCS %s\n", PROGRAM_VERSION_STRING);
    printf("---------+\n");

    INFO(("Parsing the command line."));
    if (!kcom_parse_command_line(argc, argv))
    {
        NBENE(("Command line parse failed. Exiting."));
        goto done;
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
    }
    else
    {
        kpropagate_news_of_new_capture_video_mode();
    }

    INFO(("Entering the main loop."));
    try
    {
        while (!PROGRAM_EXIT_REQUESTED)
        {
            process_next_capture_event();
            kd_spin_event_loop();

            #if !USE_RGBEASY_API
                // Reduce needless CPU use while debugging, etc.
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            #endif
        }
    }
    catch (std::exception &e)
    {
        char excStr[256];
        snprintf(excStr, NUM_ELEMENTS(excStr), "VCS has encountered a runtime error, and has decided "
                                               "that it's best to close down."
                                               "\n\nThe following error was caught:\n\"%s\"", e.what());

        kd_show_headless_error_message("Exception caught", excStr);
        NBENE(("%s", excStr));
    }

    done:
    PROGRAM_EXIT_REQUESTED = true;  // In case we got here through an exception or the like.
    cleanup_all();
    return 0;
}
#endif
