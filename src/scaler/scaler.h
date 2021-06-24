/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_SCALER_SCALER_H
#define VCS_SCALER_SCALER_H

#include "common/globals.h"
#include "common/propagate/vcs_event.h"

struct captured_frame_s;

// The parameters accepted by scaling functions.
#define SCALER_FUNC_PARAMS u8 *const pixelData, const resolution_s &sourceRes, const resolution_s &targetRes

extern vcs_event_c<void> ks_evNewFrameResolution;

// The most recent captured frame has now been processed and is ready for display.
extern vcs_event_c<void> ks_evNewFrame;

// The number of frames processed (see newFrame) in the last second.
extern vcs_event_c<unsigned> ks_evFramesPerSecond;

// IDs for the different up/downscaling filters the scaler can use.
enum scaling_filter_id_e
{
    SCALEFILT_NEAREST = 1,
    SCALEFILT_AREA,
    SCALEFILT_LANCZOS,
    SCALEFILT_LINEAR,
    SCALEFILT_CUBIC
};

// When padding to maintain a frame's aspect ratio is enabled, the aspect mode
// determines what that ratio is.
enum class aspect_mode_e
{
    native,           // Frame width / height.
    traditional_4_3,  // Frame width / height; except for some resolutions like 720 x 400, where 4 / 3.
    always_4_3        // 4 / 3.
};

struct scaling_filter_s
{
    // The public name of the filter. Shown in the GUI etc.
    std::string name;

    // The function that executes this scaler with the given pixels.
    void (*scale)(SCALER_FUNC_PARAMS);
};

resolution_s ks_scaler_output_base_resolution(void);

resolution_s ks_scaler_output_resolution(void);

bool ks_is_forced_aspect_enabled(void);

uint ks_max_output_bit_depth(void);

void ks_initialize_scaler(void);

void ks_release_scaler(void);

void ks_scale_frame(const captured_frame_s &frame);

resolution_s ks_resolution_to_aspect(const resolution_s &r);

void ks_set_aspect_mode(const aspect_mode_e mode);

aspect_mode_e ks_aspect_mode(void);

void ks_set_output_scale_override_enabled(const bool state);

void ks_set_output_base_resolution(const resolution_s &r, const bool originatesFromUser);

void ks_set_output_resolution_override_enabled(const bool state);

void ks_set_forced_aspect_enabled(const bool state);

void ks_indicate_no_signal(void);

void ks_indicate_invalid_signal(void);

void ks_clear_scaler_output_buffer(void);

const u8* ks_scaler_output_pixels_ptr(void);

const std::string &ks_upscaling_filter_name(void);

const std::string& ks_downscaling_filter_name(void);

real ks_output_scaling(void);

std::vector<std::string> ks_list_of_scaling_filter_names(void);

void ks_set_output_scaling(const real s);

void ks_set_downscaling_filter(const std::string &name);

void ks_set_upscaling_filter(const std::string &name);

const scaling_filter_s* ks_scaler_for_name_string(const std::string &name);

#endif
