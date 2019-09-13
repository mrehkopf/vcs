/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "common/memory_interface.h"
#include "display/display.h"
#include "common/globals.h"

class filter_c;
struct filter_widget_s;
struct scaling_filter_s;
struct filter_dlg_s;

// The signature of the function of a filter which applies that function to the
// given pixels.
#define FILTER_FUNC_PARAMS u8 *const pixels, const resolution_s *const r, const u8 *const params
typedef void(*filter_function_t)(FILTER_FUNC_PARAMS);

class filter_c
{
public:
    filter_c(const char *const id);
    ~filter_c();

    enum class filter_types_e
    {
        blur,
        delta_histogram,
        unique_count,
        unsharp_mask,
        decimate,
        denoise,
        denoise_nonlocal_means,
        sharpen,
        median,
        crop,
        flip,
        rotate,

        input_gate,
        output_gate,
    };

    const filter_types_e type;

    // A function used to apply this filter to a pixel buffer (frame).
    const filter_function_t apply;

    // An array containing the filter's parameter values; like radius for a blur
    // filter.
    heap_bytes_s<u8> parameterData;

    // A GUI-displayable widget containing e.g. user-interactible controls for
    // adjusting the filter's parameters.
    filter_widget_s *const guiWidget;

private:
    filter_types_e filter_type_for_id(const std::string id);
    filter_function_t filter_function_for_type(const filter_types_e type);
    filter_widget_s* filter_widget_for_type(const filter_types_e type, u8 *const parameterData);
};

// A single filter.
struct filter_s
{
    // An internal string that uniquely identifies this filter.
    std::string uuid;

    // A name string that identifies this filter in the GUI.
    std::string name;

    // A set of bytes you can cast into the filter's parameters.
    u8 data[FILTER_DATA_LENGTH];
};

// A set of filters (pre/post-scaling) for a particular input/output resolution activation.
struct filter_set_s
{
    // A user-facing, user-specified string that describes in brief this filter's
    // purpose or the like.
    std::string description = "";

    const scaling_filter_s *scaler = nullptr;     // The scaler used for up/downscaling.

    std::vector<filter_s> preFilters;   // Filters applied before scaling.
    std::vector<filter_s> postFilters;  // Filters applied after scaling.

    // Whether this filter set activates for all resolutions, a given input resolution,
    // or a given combination of input and output resolutions.
    enum activation_e : uint { none = 0, all = 1, in = 2, out = 4 };
    uint activation = activation_e::none;

    // The resolution(s) in which this filter ser operates, dependent on its
    // activation type.
    resolution_s inRes, outRes;

    bool isEnabled = true;
};

void kf_initialize_filters(void);

const filter_c* kf_create_filter(const char *const id);

std::vector<std::string> kf_filter_uuid_list(void);

void kf_clear_filters(void);

std::string kf_filter_name_for_uuid(const std::string uuid);

void kf_set_filter_set_enabled(const uint idx, const bool isEnabled);

void kf_add_filter_set(filter_set_s *const newSet);

void kf_filter_set_swap_upward(const uint idx);

void kf_filter_set_swap_downward(const uint idx);

void kf_remove_filter_set(const uint idx);

const std::vector<filter_set_s*>& kf_filter_sets(void);

const scaling_filter_s* kf_current_filter_set_scaler(void);

void kf_release_filters(void);

void kf_set_filtering_enabled(const bool enabled);

const filter_set_s* kf_current_filter_set(void);

std::vector<int> kf_find_capture_alignment(u8 *const pixels, const resolution_s &r);

int kf_current_filter_set_idx(void);

void kf_apply_pre_filters(u8 *const pixels, const resolution_s &r);

void kf_apply_post_filters(u8 *const pixels, const resolution_s &r);

filter_function_t kf_filter_function_ptr_for_uuid(const std::string &name);

std::string kf_filter_uuid_for_name(const std::string &name);

const filter_dlg_s *kf_filter_dialog_for_uuid(const std::string &name);

bool kf_filter_exists(const std::string &name);

#endif
