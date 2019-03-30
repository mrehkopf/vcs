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

#include "propagate.h"
#include "../common/globals.h"
#include "../display/display.h"
#include "../capture/capture.h"
#include "../scaler/scaler.h"
#include "../record/record.h"

// A new input video mode (e.g. resolution) has been set.
void kpropagate_new_input_video_mode(void)
{
    const input_signal_s s = kc_hardware().status.signal();

    if (s.wokeUp)
    {
        kpropagate_gained_input_signal();
    }

    kc_apply_new_capture_resolution();

    kd_update_gui_input_signal_info();

    ks_set_output_base_resolution(s.r, false);

    kd_update_display();

    return;
}

// Call to ask the capture's horizontal and/or vertical positioning to be
// adjusted by the given amount.
void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta)
{
    kc_adjust_capture_horizontal_offset(horizontalDelta);

    kc_adjust_capture_vertical_offset(verticalDelta);

    return;
}

// The capture hardware received an invalid input signal.
void kpropagate_invalid_input_signal(void)
{
    kd_mark_gui_input_no_signal(true);

    ks_indicate_invalid_signal();

    kd_update_display();

    return;
}

// The capture hardware lost its input signal.
void kpropagate_lost_input_signal(void)
{
    kd_mark_gui_input_no_signal(true);

    ks_indicate_no_signal();

    kd_update_display();

    return;
}

// The capture hardware started receiving an input signal.
void kpropagate_gained_input_signal(void)
{
    kd_mark_gui_input_no_signal(false);

    return;
}

// The capture hardware has sent us a new captured frame.
void kpropagate_new_captured_frame(void)
{
    ks_scale_frame(kc_latest_captured_frame());

    if (krecord_is_recording())
    {
        krecord_record_new_frame();
    }

    kc_mark_frame_buffer_as_processed();

    kd_update_display();

    return;
}

// The capture hardware has met with an unrecoverable error.
void kpropagate_unrecoverable_error(void)
{
    NBENE(("VCS has met with an unrecoverable error. Shutting the program down."));

    PROGRAM_EXIT_REQUESTED = true;

    return;
}
