/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef SCALER_H
#define SCALER_H

#include "../common/common.h"

class QImage;

struct captured_frame_s;

// The parameters accepted by scaling functions.
#define SCALER_FUNC_PARAMS u8 *const pixelData, const resolution_s &sourceRes, const resolution_s &targetRes

// IDs for the different up/downscaling filters the scaler can use.
enum scaling_filter_id_e
{
    SCALEFILT_NEAREST = 1,
    SCALEFILT_AREA,
    SCALEFILT_LANCZOS,
    SCALEFILT_LINEAR,
    SCALEFILT_CUBIC
};

struct scaling_filter_s
{
    QString name;   // The public name of the filter. Shown in the GUI etc.
    void (*scale)(SCALER_FUNC_PARAMS);  // The function that executes this scaler with the given pixels.
};

resolution_s ks_output_base_resolution(void);

resolution_s ks_output_resolution(void);

bool ks_is_output_padding_enabled(void);

resolution_s ks_padded_output_resolution(void);

uint ks_max_output_bit_depth(void);

void ks_initialize_scaler(void);

void ks_release_scaler(void);

void ks_scale_frame(captured_frame_s &frame);

resolution_s ks_resolution_to_aspect_ratio(const resolution_s &r);

void ks_set_output_aspect_ratio(const u32 w, const u32 h);

void ks_set_output_aspect_ratio_override_enabled(const bool state);

void ks_set_output_scale_override_enabled(const bool state);

void ks_set_output_base_resolution(const resolution_s &r, const bool originatesFromUser);

void ks_set_output_resolution_override_enabled(const bool state);

void ks_set_output_pad_resolution(const resolution_s &r);

void ks_set_output_pad_override_enabled(const bool state);

void ks_indicate_no_signal(void);

void ks_indicate_invalid_signal(void);

void ks_clear_scaler_output_buffer(void);

const u8* ks_scaler_output_as_raw_ptr(void);

QImage ks_scaler_output_buffer_as_qimage(void);

const QString& ks_upscaling_filter_name(void);

const QString& ks_downscaling_filter_name(void);

real ks_output_scaling(void);

QStringList ks_list_of_scaling_filter_names(void);

void ks_set_output_scaling(const real s);

void ks_set_downscaling_filter(const QString &name);

void ks_set_upscaling_filter(const QString &name);

const scaling_filter_s* ks_scaler_for_name_string(const QString &name);

resolution_s ks_scaler_output_resolution(void);

resolution_s ks_scaler_output_aspect_ratio(void);

#endif
