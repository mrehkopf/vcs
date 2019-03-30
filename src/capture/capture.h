/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <vector>
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

enum class capture_event_e
{
    none,
    sleep,
    no_signal,
    new_frame,
    new_video_mode,
    invalid_signal,
    unrecoverable_error
};

struct captured_frame_s
{
    resolution_s r;

    heap_bytes_s<u8> pixels;

    // Will be set to true after the frame has been processed (i.e. scaled, filtered, etc.).
    bool processed = false;
};

struct capture_signal_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
    bool wokeUp;
};

struct input_video_settings_s
{
    unsigned long horizontalScale;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;
};

struct input_color_settings_s
{
    long brightness;
    long contrast;

    long redBrightness;
    long redContrast;

    long greenBrightness;
    long greenContrast;

    long blueBrightness;
    long blueContrast;
};

// For each input mode (i.e. resolution), the set of parameters that the user
// has defined.
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

// Functions that provide information about the capture hardware. These functions
// generally poll the hardware directly, using the RGBEasy API (although whether
// the API actually polls the hardware for each call is another matter, but you
// get the point).
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

// Initialize and release the unit.
void kc_initialize_capture(void);
void kc_release_capture(void);

// Public getters.
int kc_index_of_alias_resolution(const resolution_s r);
uint kc_num_missed_frames(void);
uint kc_input_channel_idx(void);
uint kc_output_color_depth(void);
uint kc_input_color_depth(void);
bool kc_are_frames_being_missed(void);
bool kc_is_capture_active(void);
bool kc_is_aliased_resolution(void);
bool kc_should_current_frame_be_skipped(void);
bool kc_is_invalid_signal(void);
bool kc_no_signal(void);
PIXELFORMAT kc_pixel_format(void);
const capture_hardware_s& kc_hardware(void);
capture_event_e kc_latest_capture_event(void);
const captured_frame_s& kc_latest_captured_frame(void);
mode_params_s kc_mode_params_for_resolution(const resolution_s r);

// Public setters.
bool kc_set_resolution(const resolution_s r);
bool kc_set_mode_parameters_for_resolution(const resolution_s r);
bool kc_set_frame_dropping(const u32 drop);
bool kc_set_input_channel(const u32 channel);
bool kc_set_input_color_depth(const u32 bpp);
void kc_set_color_settings(const input_color_settings_s c);
void kc_set_video_settings(const input_video_settings_s v);
void kc_mark_current_frame_as_processed(void);
void kc_reset_missed_frames_count(void);
bool kc_adjust_video_vertical_offset(const int delta);
bool kc_adjust_video_horizontal_offset(const int delta);
void kc_apply_new_capture_resolution(void);
#if VALIDATION_RUN
    void kc_VALIDATION_set_capture_color_depth(const uint bpp);
    void kc_VALIDATION_set_capture_pixel_format(const PIXELFORMAT pf);
#endif

// Miscellaneous functions, to be sorted.
void kc_broadcast_aliases_to_gui(void);
void kc_broadcast_mode_params_to_gui(void);
void kc_insert_test_image(void);
void kc_update_alias_resolutions(const std::vector<mode_alias_s> &aliases);
bool kc_save_mode_params(const QString filename);
bool kc_load_mode_params(const QString filename, const bool automaticCall);
bool kc_load_aliases(const QString filename, const bool automaticCall);
bool kc_save_aliases(const QString filename);

#endif
