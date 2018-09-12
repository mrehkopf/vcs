/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "display.h"
#include "common.h"

struct scaling_filter_s;
struct filter_dlg_s;

// The signature of the function of a filter which applies that function to the
// given pixels.
#define FILTER_FUNC_PARAMS u8 *const pixels, const resolution_s *const r, const u8 *const params
typedef void(*filter_function_t)(FILTER_FUNC_PARAMS);

// A single filter.
struct filter_s
{
    // A string that uniquely identifies this filter. Will also be shown in the
    // GUI, i.e. is user-facing.
    QString name;

    // A set of bytes you can cast into the filter's parameters.
    u8 data[FILTER_DATA_LENGTH];
};

// A collection of filters (pre/post-scaling) for a particular input/output
// resolution set.
struct filter_complex_s
{
    const scaling_filter_s *scaler;     // The scaler used for up/downscaling.

    std::vector<filter_s> preFilters;   // Filters applied before scaling.
    std::vector<filter_s> postFilters;  // Filters applied after scaling.

    resolution_s inRes, outRes;         // The resolutions for which this filter operates.

    bool isEnabled;
};

void kf_initialize_filters(void);

QStringList kf_filter_name_list(void);

void kf_clear_filters(void);

const scaling_filter_s *kf_current_filter_complex_scaler(void);

const filter_complex_s* kf_filter_complex_for_resolutions(const resolution_s &in, const resolution_s &out);

void kf_release_filters(void);

const std::vector<filter_complex_s> &kf_filter_complexes(void);

void kf_set_filter_complex_enabled(const bool enabled,
                                   const resolution_s &inRes, const resolution_s &outRes);

void kf_set_filtering_enabled(const bool enabled);

void kf_apply_pre_filters(u8 *const pixels, const resolution_s &r);

void kf_apply_post_filters(u8 *const pixels, const resolution_s &r);

void kf_update_filter_complex(const filter_complex_s &f);

void kf_activate_filter_complex_for(const resolution_s &inR, const resolution_s &outR);

// Returns a pointer to the filter function that corresponds to the given filter
// name string.
//
filter_function_t kf_filter_function_ptr_for_name(const QString &name);

const filter_dlg_s *kf_filter_dialog_for_name(const QString &name);

bool kf_named_filter_exists(const QString &name);

#endif
