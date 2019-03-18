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
#include "display/display.h"
#include "record/record.h"
#include "memory.h"
#include "scaler/scaler.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "main.h"

// Set to !0 when we want to exit the program.
/// TODO. Don't have this global.
i32 PROGRAM_EXIT_REQUESTED = 0;

extern std::mutex INPUT_OUTPUT_MUTEX;

static void cleanup_all(void)
{
    INFO(("Received orders to exit. Initiating cleanup."));

    kd_release_display();
    ks_release_scaler();
    kc_release_capturer();
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
    if (!PROGRAM_EXIT_REQUESTED) kc_initialize_capturer();
    if (!PROGRAM_EXIT_REQUESTED) kat_initialize_anti_tear();
    if (!PROGRAM_EXIT_REQUESTED) kf_initialize_filters();

    // Ideally, do these last.
    if (!PROGRAM_EXIT_REQUESTED)
    {
        kd_acquire_display();
        kc_broadcast_aliases_to_gui();
        kc_broadcast_mode_params_to_gui();
    }

    return !PROGRAM_EXIT_REQUESTED;
}

// Updates all necessary units with information about the current (new) capture
// input mode.
//
void kmain_signal_new_input_mode(void)
{
    const input_signal_s &s = kc_input_signal_info();

    kc_apply_new_capture_resolution(s.r);
    kd_update_gui_input_signal_info(s);
    ks_set_output_base_resolution(s.r, false);

    return;
}

// Try to have the capture card force the given input resolution.
//
void kmain_change_capture_input_resolution(const resolution_s r)
{
    std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

    const resolution_s min = kc_hardware_min_capture_resolution();
    const resolution_s max = kc_hardware_max_capture_resolution();

    if (kc_no_signal())
    {
        DEBUG(("Was asked to change the input resolution while the capture card was not receiving a signal. Ignoring the request."));
        goto done;
    }

    if (r.w > max.w ||
        r.w < min.w ||
        r.h > max.h ||
        r.h < min.h)
    {
        NBENE(("Was asked to set an input resolution which is not supported by the capture card (%u x %u). Ignoring the request.",
               r.w, r.h));
        goto done;
    }

    if (!kc_force_capture_input_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        goto done;
    }

    kmain_signal_new_input_mode();

    done:
    return;
}

static capture_event_e process_next_capture_event(void)
{
    std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

    #if USE_RGBEASY_API
        const capture_event_e e = kc_get_next_capture_event();
    #else
        const capture_event_e e = CEVENT_NEW_FRAME;
        kc_insert_test_image();
    #endif

    switch (e)
    {
        case CEVENT_UNRECOVERABLE_ERROR:
        {
            NBENE(("The capture card reports an unrecoverable error. Shutting down the program."));

            PROGRAM_EXIT_REQUESTED = true;

            break;
        }
        case CEVENT_NEW_FRAME:
        {
            ks_scale_frame(kc_latest_captured_frame());

            break;
        }
        case CEVENT_NEW_VIDEO_MODE:
        {
            if (kc_input_signal_info().wokeUp)
            {
                kd_mark_gui_input_no_signal(false);
            }

            kmain_signal_new_input_mode();

            break;
        }
        case CEVENT_NO_SIGNAL:
        {
            kd_mark_gui_input_no_signal(true);

            ks_indicate_no_signal();

            break;
        }
        case CEVENT_INVALID_SIGNAL:
        {
            kd_mark_gui_input_no_signal(true);

            ks_indicate_invalid_signal();

            break;
        }
        case CEVENT_SLEEP:
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(4)); /// TODO. Is 4 the best wait-time?

            break;
        }
        case CEVENT_NONE:
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

    if ((e != CEVENT_SLEEP) &&
        (e != CEVENT_NONE) &&
        (e != CEVENT_UNRECOVERABLE_ERROR))
    {
        kd_update_display();

        if (e == CEVENT_NEW_FRAME)
        {
            if (krecord_is_recording()) krecord_record_new_frame();

            kc_mark_frame_buffer_as_processed();
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

    INFO(("Entering the main loop."));
    try
    {
        while (!PROGRAM_EXIT_REQUESTED)
        {
            process_next_capture_event();
            kd_process_ui_events();

            #if !USE_RGBEASY_API
                // Reduce needless CPU use while debugging, etc.
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            #endif
        }
    }
    catch (std::exception &e)
    {
        char excStr[256];
        snprintf(excStr, NUM_ELEMENTS(excStr), "VCS has encountered a runtime error, and decided "
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
