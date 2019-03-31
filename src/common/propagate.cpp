/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS event propagation
 *
 * Provides functions for propagating various events. For instance, when a new
 * frame is captured, you can call the appropriate propagation function offered
 * here to have VCS take the necessary actions to deal with the frame, like
 * scaling it and painting it on the screen.
 *
 */

#include <mutex>
#include "propagate.h"
#include "../common/globals.h"
#include "../display/display.h"
#include "../capture/capture.h"
#include "../scaler/scaler.h"
#include "../record/record.h"

extern std::mutex INPUT_OUTPUT_MUTEX;

// A new input video mode (e.g. resolution) has been set.
void kpropagate_news_of_new_capture_video_mode(void)
{
    const capture_signal_s s = kc_hardware().status.signal();

    if (s.wokeUp)
    {
        kpropagate_news_of_gained_capture_signal();
    }

    kc_apply_new_capture_resolution();

    kd_update_gui_capture_signal_info();

    ks_set_output_base_resolution(s.r, false);

    kd_update_display();

    return;
}

// Call to let the system know that the given mode parameters have been loaded
// from the given file.
void kpropagate_loaded_mode_params_from_disk(const std::vector<mode_params_s> &modeParams,
                                             const std::string &sourceFilename)
{
    kc_set_mode_params(modeParams);

    kc_broadcast_mode_params_to_gui();

    // In case the mode params changed for the current mode, re-initialize it.
    kpropagate_news_of_new_capture_video_mode();

    kd_signal_new_mode_settings_source_file(sourceFilename);

    INFO(("Loaded %u set(s) of mode params from disk.", modeParams.size()));

    return;
}

void kpropagate_saved_mode_params_to_disk(const std::vector<mode_params_s> &modeParams,
                                          const std::string &targetFilename)
{
    INFO(("Saved %u set(s) of mode params to disk.", modeParams.size()));

    kd_signal_new_mode_settings_source_file(targetFilename);

    return;
}

void kpropagate_saved_aliases_to_disk(const std::vector<mode_alias_s> &aliases,
                                      const std::string &targetFilename)
{
    INFO(("Saved %u aliases to disk.", aliases.size()));

    return;
}

void kpropagate_loaded_aliases_from_disk(const std::vector<mode_alias_s> &aliases,
                                         const std::string &sourceFilename)
{
    kc_set_aliases(aliases);

    kc_broadcast_aliases_to_gui();

    INFO(("Loaded %u alias set(s) from disk.", aliases.size()));

    return;
}

// Call to ask the capture's horizontal and/or vertical positioning to be
// adjusted by the given amount.
void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta)
{
    kc_adjust_video_horizontal_offset(horizontalDelta);

    kc_adjust_video_vertical_offset(verticalDelta);

    return;
}

// The capture hardware received an invalid input signal.
void kpropagate_news_of_invalid_capture_signal(void)
{
    kd_notify_gui_of_capture_signal_change(true);

    ks_indicate_invalid_signal();

    kd_update_display();

    return;
}

// The capture hardware lost its input signal.
void kpropagate_news_of_lost_capture_signal(void)
{
    kd_notify_gui_of_capture_signal_change(true);

    ks_indicate_no_signal();

    kd_update_display();

    return;
}

// The capture hardware started receiving an input signal.
void kpropagate_news_of_gained_capture_signal(void)
{
    kd_notify_gui_of_capture_signal_change(false);

    return;
}

// The capture hardware has sent us a new captured frame.
void kpropagate_news_of_new_captured_frame(void)
{
    ks_scale_frame(kc_latest_captured_frame());

    if (krecord_is_recording())
    {
        krecord_record_new_frame();
    }

    kc_mark_current_frame_as_processed();

    kd_update_display();

    return;
}

// The capture hardware has met with an unrecoverable error.
void kpropagate_news_of_unrecoverable_error(void)
{
    NBENE(("VCS has met with an unrecoverable error. Shutting the program down."));

    PROGRAM_EXIT_REQUESTED = true;

    return;
}

void kpropagate_forced_capture_resolution(const resolution_s &r)
{
    std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

    const resolution_s min = kc_hardware().meta.minimum_capture_resolution();
    const resolution_s max = kc_hardware().meta.maximum_capture_resolution();

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

    if (!kc_set_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        goto done;
    }

    kpropagate_news_of_new_capture_video_mode();

    done:

    return;
}
