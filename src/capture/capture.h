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
#include "../display/display.h"
#include "../scaler/scaler.h"
#include "../common/memory.h"
#include "../common/globals.h"
#include "../common/types.h"

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

struct capture_signal_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
    bool wokeUp;        // Set to true if there was a 'no signal' before this.
};

struct input_video_settings_s
{
    unsigned long horScale;
    long horPos;
    long verPos;
    long phase;
    long blackLevel;
};

struct input_color_settings_s
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
    input_color_settings_s color;
    input_video_settings_s video;
};

struct mode_alias_s
{
    resolution_s from;
    resolution_s to;
};

// Information about the capture hardware. These functions generally poll the
// hardware directly, using the RGBEasy API.
//
// NOTE: These functions will be exposed to the entire program, so they should
// not provide any means (e.g. calls to RGBSet*()) to alter the state of the
// capture hardware - e.g. to start or stop capture, change the settings, etc.
struct capture_hardware_s
{
    struct features_supported_s
    {
        bool component_capture(void) const;
        bool composite_capture(void) const;
        bool deinterlace(void) const;
        bool dma(void) const;
        bool dvi(void) const;
        bool svideo(void) const;
        bool vga(void) const;
        bool yuv(void) const;
    } supports;

    struct metainfo_s
    {
        input_color_settings_s default_color_settings(void) const;
        input_color_settings_s minimum_color_settings(void) const;
        input_color_settings_s maximum_color_settings(void) const;
        input_video_settings_s default_video_settings(void) const;
        input_video_settings_s minimum_video_settings(void) const;
        input_video_settings_s maximum_video_settings(void) const;
        resolution_s minimum_capture_resolution(void) const;
        resolution_s maximum_capture_resolution(void) const;
        std::string firmware_version(void) const;
        std::string driver_version(void) const;
        std::string model_name(void) const;
        int minimum_frame_drop(void) const;
        int maximum_frame_drop(void) const;
        int num_capture_inputs(void) const;
        bool is_dma_enabled(void) const;
    } meta;

    struct status_s
    {
        input_color_settings_s color_settings(void) const;
        input_video_settings_s video_settings(void) const;
        resolution_s capture_resolution(void) const;
        capture_signal_s signal(void) const;
        int frame_rate(void) const;
    } status;
};

const capture_hardware_s& kc_hardware(void);

captured_frame_s &kc_latest_captured_frame(void);

bool kc_are_frames_being_missed(void);

uint kc_num_missed_frames(void);

void kc_insert_test_image(void);

void kc_reset_missed_frames_count(void);

void kc_initialize_capture(void);

void kc_release_capture(void);

bool kc_adjust_capture_vertical_offset(const int delta);

bool kc_adjust_capture_horizontal_offset(const int delta);

uint kc_current_capture_input_channel_idx(void);

bool kc_pause_capture(void);

bool kc_resume_capture(void);

bool kc_is_capture_active(void);

bool kc_load_aliases_(const QString filename, const bool automatic);

bool kc_force_capture_resolution(const resolution_s r);

int kc_index_of_alias_resolution(const resolution_s r);

mode_params_s kc_mode_params_for_resolution(const resolution_s r);

bool kc_apply_mode_parameters(const resolution_s r);

void kc_apply_new_capture_resolution(void);

bool kc_is_aliased_resolution(void);

void kc_mark_frame_buffer_as_processed(void);

bool kc_should_skip_next_frame(void);

bool kc_no_signal(void);

bool kc_is_invalid_signal(void);

capture_event_e kc_get_next_capture_event(void);

bool kc_set_capture_frame_dropping(const u32 drop);

void kc_set_capture_video_params(const input_video_settings_s p);

void kc_set_capture_color_params(const input_color_settings_s c);

u32 kc_capture_frame_rate(void);

bool kc_set_capture_input_channel(const u32 channel);

bool kc_set_output_bit_depth(const u32 bpp);

PIXELFORMAT kc_output_pixel_format(void);

u32 kc_capture_color_depth(void);

u32 kc_output_bit_depth(void);

void kc_broadcast_aliases_to_gui(void);

void kc_update_alias_resolutions(const std::vector<mode_alias_s> &aliases);

bool kc_load_aliases(const QString filename, const bool automaticCall);

bool kc_save_aliases(const QString filename);

bool kc_save_mode_params(const QString filename);

void kc_broadcast_mode_params_to_gui(void);

bool kc_load_mode_params(const QString filename, const bool automaticCall);

#endif
