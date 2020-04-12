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
#include "capture/capture_api.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "capture/alias.h"
#include "scaler/scaler.h"
#include "record/record.h"

// A new input video mode (e.g. resolution) has been set.
void kpropagate_news_of_new_capture_video_mode(void)
{
    /// TODO: We only need to do this if this new video mode follows a period of
    /// no video mode, i.e. no signal.
    kpropagate_news_of_gained_capture_signal();

    resolution_s resolution = kc_capture_api().get_resolution();

    INFO(("New video mode: %u x %u @ %f Hz.", resolution.w, resolution.h, kc_capture_api().get_refresh_rate().value<double>()));

    if (ka_has_alias(resolution))
    {
        const resolution_s aliasResolution = ka_aliased(resolution);

        if (kc_capture_api().set_resolution(aliasResolution))
        {
            resolution = aliasResolution;

            INFO((" -> Aliased to %u x %u.", aliasResolution.w, aliasResolution.h));
        }
        else
        {
            INFO((" -> This mode has an alias, but it could not be set."));
        }
    }

    kvideopreset_apply_current_active_preset();

    kd_update_capture_signal_info();

    ks_set_output_base_resolution(resolution, false);

    kd_update_output_window_size();

    kd_update_output_window_title();

    kd_redraw_output_window();

    return;
}

void kpropagate_news_of_changed_input_channel(void)
{
    kd_update_capture_signal_info();

    return;
}

// Recording of output frames into a video file has begun. This message should be
// propagated only when the recording has started anew; so e.g. resuming from a
// paused state should not propagate this message.
void kpropagate_news_of_recording_started(void)
{
    kd_set_video_recording_is_active(true);
    kd_update_video_recording_metainfo();
    kd_update_output_window_title();

    // Disable any GUI functionality that would let the user change the current
    // output size, since we want to keep the output resolution constant while
    // recording.
    kd_disable_output_size_controls(true);

    return;
}

// Recording of output frames into a video file has ended. Note that suspended states,
// like the recording being paused, should not propagate this message.
void kpropagate_news_of_recording_ended(void)
{
    kd_disable_output_size_controls(false);

    kd_set_video_recording_is_active(false);
    kd_update_video_recording_metainfo();
    kd_update_output_window_title();

    // The output resolution might have changed while we were recording, but
    // since we also prevent the size of the output window from changing while
    // recording, we should now - that recording has stopped - tell the window
    // to update its size to match the current proper output size.
    kd_update_output_window_size();

    return;
}

// Call to let the system know that the given video presets have been loaded
// from the given file.
void kpropagate_loaded_video_presets_from_disk(const std::vector<video_preset_s*> &presets,
                                               const std::string &sourceFilename)
{
    kvideopreset_assign_presets(presets);

    // In case we loaded in parameters for the current resolution.
    ///kc_capture_api().set_video_signal_parameters(kvideoparam_parameters_for_resolution(kc_capture_api().get_resolution()));

    kd_update_video_mode_params();
    kd_set_video_presets_filename(sourceFilename);

    INFO(("Loaded %u preset(s).", presets.size()));

    return;
}

void kpropagate_saved_video_presets_to_disk(const std::vector<video_preset_s*> &presets,
                                            const std::string &targetFilename)
{
    INFO(("Saved %u video preset(s).", presets.size()));

    kd_set_video_presets_filename(targetFilename);

    return;
}

void kpropagate_saved_filter_graph_to_disk(const std::string &targetFilename)
{
    INFO(("Saved the filter graph."));

    kd_set_filter_graph_source_filename(targetFilename);

    return;
}

void kpropagate_saved_aliases_to_disk(const std::vector<mode_alias_s> &aliases,
                                      const std::string &targetFilename)
{
    INFO(("Saved %u aliases.", aliases.size()));

    (void)targetFilename;

    return;
}

void kpropagate_loaded_aliases_from_disk(const std::vector<mode_alias_s> &aliases,
                                         const std::string &sourceFilename)
{
    ka_set_aliases(aliases);

    ka_broadcast_aliases_to_gui();

    INFO(("Loaded %u alias(es).", aliases.size()));

    (void)sourceFilename;

    return;
}

void kpropagate_loaded_filter_graph_from_disk(const std::vector<FilterGraphNode*> &nodes,
                                              const std::vector<filter_graph_option_s> &graphOptions,
                                              const std::string &sourceFilename)
{
    kd_set_filter_graph_source_filename(sourceFilename);

    kd_set_filter_graph_options(graphOptions);

    (void)sourceFilename;
    (void)nodes;

    return;
}

// The capture hardware received an invalid input signal.
void kpropagate_news_of_invalid_capture_signal(void)
{
    kd_set_capture_signal_reception_status(false);

    ks_indicate_invalid_signal();

    kd_redraw_output_window();

    return;
}

// The capture hardware lost its input signal and thus enters a state of no signal.
void kpropagate_news_of_lost_capture_signal(void)
{
    INFO(("No signal."));

    kd_set_capture_signal_reception_status(false);

    ks_indicate_no_signal();

    kd_redraw_output_window();

    return;
}

// The capture hardware started receiving an input signal after being in a state
// of no signal.
void kpropagate_news_of_gained_capture_signal(void)
{
    kd_set_capture_signal_reception_status(true);

    return;
}

// The capture hardware has sent us a new captured frame.
void kpropagate_news_of_new_captured_frame(void)
{
    ks_scale_frame(kc_capture_api().get_frame_buffer());

    if (krecord_is_recording())
    {
        krecord_record_new_frame();
    }

    kc_capture_api().mark_frame_buffer_as_processed();

    kd_redraw_output_window();

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
    const resolution_s min = kc_capture_api().get_minimum_resolution();
    const resolution_s max = kc_capture_api().get_maximum_resolution();

    if (kc_capture_api().has_no_signal())
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

    if (!kc_capture_api().set_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        goto done;
    }

    kpropagate_news_of_new_capture_video_mode();

    done:

    return;
}
