/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <QString>
#include <vector>
#include <atomic>
#include "display.h"
#include "scaler.h"
#include "memory.h"
#include "common.h"
#include "types.h"

#if USE_RGBEASY_API
    #include <windows.h>
    #include <rgb.h>
    #include <rgbapi.h>
    #include <rgberror.h>
#else
    #include "null_rgbeasy.h"
#endif

enum capture_event_e
{
    CEVENT_NONE,
    CEVENT_NEW_FRAME,
    CEVENT_NEW_VIDEO_MODE,
    CEVENT_NO_SIGNAL,
    CEVENT_INVALID_SIGNAL,
    CEVENT_SLEEP,
    CEVENT_UNRECOVERABLE_ERROR
};

struct captured_frame_s
{
    resolution_s r;
    heap_bytes_s<u8> pixels;
    bool processed = false;     // Set to true after the frame has been processed for display on the screen (i.e. scaled, filtered, etc.).

    // Returns the number of pixels used up by the frame given its current
    // resolution. DOES NOT return the size of the pixel buffer, since the
    // resolution can be changed independently.
    uint num_pixel_bytes(void) const
    {
        return (r.w * r.h * (r.bpp / 8));
    }
};

struct input_signal_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
    bool wokeUp;        // Set to true if there was a 'no signal' before this.
};

struct input_video_params_s
{
    unsigned long horScale;
    long horPos;
    long verPos;
    long phase;
    long blackLevel;
};

struct input_color_params_s
{
    long bright;
    long contr;

    long redBright;
    long redContr;

    long greenBright;
    long greenContr;

    long blueBright;
    long blueContr;
};

// For each input mode (i.e. resolution), the set of parameters that the user has
// defined.
struct mode_params_s
{
    resolution_s r;
    input_color_params_s color;
    input_video_params_s video;
};

struct mode_alias_s
{
    resolution_s from;
    resolution_s to;
};

captured_frame_s &kc_latest_captured_frame(void);

bool kc_capturer_has_missed_frames(void);

uint kc_missed_input_frames_count(void);

void kc_reset_missed_frames_count(void);

void kc_initialize_capturer(void);

void kc_release_capturer(void);

bool kc_is_direct_dma_enabled(void);

u32 kc_hardware_min_frame_drop(void);

u32 kc_hardware_max_frame_drop(void);

bool kc_start_capture(void);

u32 kc_input_channel_idx(void);

bool kc_hardware_is_yuv_supported(void);

bool kc_hardware_is_deinterlace_supported(void);

bool kc_hardware_is_direct_dma_supported(void);

bool kc_hardware_is_svideo_capture_supported(void);

bool kc_hardware_is_dvi_capture_supported(void);

bool kc_hardware_is_component_capture_supported(void);

bool kc_hardware_is_composite_capture_supported(void);

bool kc_hardware_is_vga_capture_supported(void);

QString kc_capture_card_type_string(void);

bool kc_pause_capture(void);

bool kc_resume_capture(void);

bool kc_is_capture_active(void);

u32 kc_hardware_num_capture_inputs(void);

const resolution_s &kc_hardware_max_capture_resolution(void);

const resolution_s &kc_hardware_min_capture_resolution(void);

bool kc_load_aliases_(const QString filename, const bool automatic);

resolution_s kc_input_resolution(void);

bool kc_force_capture_input_resolution(const resolution_s r);

int kc_alias_resolution_index(const resolution_s r);

mode_params_s kc_mode_params_for_resolution(const resolution_s r);

bool kc_apply_mode_parameters(const resolution_s r);

void kc_apply_new_capture_resolution(resolution_s r);

bool kc_is_aliased_resolution(void);

void kc_mark_frame_buffer_as_processed(void);

bool kc_should_skip_next_frame(void);

bool kc_is_no_signal(void);

bool kc_is_invalid_signal(void);

capture_event_e kc_get_next_capture_event(void);

bool kc_set_capture_frame_dropping(const u32 drop);

void kc_set_capture_video_params(const input_video_params_s p);

void kc_set_capture_color_params(const input_color_params_s c);

input_video_params_s kc_capture_video_params(void);

input_video_params_s kc_capture_video_params_min_values(void);

input_video_params_s kc_capture_video_params_default_values(void);

input_video_params_s kc_capture_video_params_max_values(void);

input_color_params_s kc_capture_color_params(void);

input_color_params_s kc_capture_color_params_default_values(void);

input_color_params_s kc_capture_color_params_min_values(void);

input_color_params_s kc_capture_color_params_max_values(void);

u32 kc_capture_frame_rate(void);

const input_signal_s &kc_input_signal_info(void);

QString kc_capture_input_signal_type_string(void);

QString kc_hardware_firmware_version_string(void);

QString kc_hardware_driver_version_string(void);

bool kc_set_capture_input_channel(const u32 channel);

bool kc_initialize_capture_card(void);

bool kc_set_output_bit_depth(const u32 bpp);

PIXELFORMAT kc_output_pixel_format(void);

u32 kc_capture_color_depth(void);

u32 kc_output_bit_depth(void);

void kc_broadcast_aliases_to_gui(void);

bool kc_load_aliases(const QString filename, const bool automaticCall);

bool kc_save_mode_params(const QString filename);

void kc_broadcast_mode_params_to_gui(void);

bool kc_load_mode_params(const QString filename, const bool automaticCall);

#endif
