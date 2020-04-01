/*
 * 2018, 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef FILTER_H_
#define FILTER_H_

#include <functional>
#include "common/memory/memory_interface.h"
#include "display/display.h"
#include "common/globals.h"

struct filter_widget_s;

// The parameters accepted by a filter's apply function (the function that
// applies a filter's processing onto the target pixels).
#define FILTER_FUNC_PARAMS u8 *const pixels, const resolution_s *const r, const u8 *const params

enum class filter_type_enum_e
{
    blur,
    delta_histogram,
    unique_count,
    unsharp_mask,
    decimate,
    denoise_temporal,
    denoise_nonlocal_means,
    sharpen,
    median,
    crop,
    flip,
    rotate,

    input_gate,
    output_gate,
};

// An image filter - applies a pre-set effect (blur, sharpening, etc.) onto
// the pixels of a captured frame.
class filter_c
{
public:
    filter_c(const std::string &id, const u8 *initialParameterValues = nullptr);
    ~filter_c();

    struct filter_metadata_s
    {
        // The filter's user-facing display name; e.g. "Blur" for a blur filter. This
        // will be shown in the GUI.
        std::string name;

        filter_type_enum_e type;

        // A function that applies the filter's processing to a frame's pixels.
        std::function<void(FILTER_FUNC_PARAMS)> apply;
    };

    const filter_metadata_s &metaData;

    // An array containing the filter's parameter values; like radius for a blur
    // filter.
    heap_bytes_s<u8> parameterData;

    // The filter's GUI widget, which provides the user with visual controls for
    // adjusting the filter's parameters.
    filter_widget_s *const guiWidget;

private:
    // Creates and returns an instance of a GUI widget for this filter.
    filter_widget_s* create_gui_widget(const u8 *const initialParameterValues = nullptr);
};

void kf_initialize_filters(void);

void kf_add_filter_chain(std::vector<const filter_c*> newChain);

void kf_remove_all_filter_chains(void);

std::string kf_filter_name_for_type(const filter_type_enum_e type);

std::string kf_filter_name_for_id(const std::string id);

std::string kf_filter_id_for_type(const filter_type_enum_e type);

filter_type_enum_e kf_filter_type_for_id(const std::string id);

void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r);

std::vector<const filter_c::filter_metadata_s*> kf_known_filter_types(void);

const filter_c* kf_create_new_filter_instance(const char *const id);
const filter_c* kf_create_new_filter_instance(const filter_type_enum_e type, const u8 *const initialParameterValues = nullptr);

void kf_delete_filter_instance(const filter_c *const filter);

void kf_release_filters(void);

void kf_set_filtering_enabled(const bool enabled);

bool kf_is_filtering_enabled(void);

std::vector<int> kf_find_capture_alignment(u8 *const pixels, const resolution_s &r);

#endif
