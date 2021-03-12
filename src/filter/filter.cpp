    /*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <unordered_map>
#include <cstring>
#include <vector>
#include <cmath>
#include <map>
#include "display/qt/widgets/filter_widgets.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "filter/filter_funcs.h"

// Whether filters (if any are activated) should be applied to incoming frames.
static bool FILTERING_ENABLED = false;

// All filter types available to the user.
//
// Note: Each filter is identified by a UUID string. A UUID must be unique to a filter.
// The UUID of a filter must never be changed - a filter must remain identifiable with
// its initially-set UUID.
static const std::unordered_map<std::string, const filter_c::filter_metadata_s> KNOWN_FILTER_TYPES =
{
    {"a5426f2e-b060-48a9-adf8-1646a2d3bd41",
    {"Blur",
     filter_category_e::reduce,
     filter_type_enum_e::blur,
     filter_func_blur}},

    {"fc85a109-c57a-4317-994f-786652231773",
    {"Delta histogram",
     filter_category_e::meta,
     filter_type_enum_e::delta_histogram,
     filter_func_delta_histogram}},

    {"badb0129-f48c-4253-a66f-b0ec94e225a0",
    {"Framerate estimate",
     filter_category_e::meta,
     filter_type_enum_e::unique_count,
     filter_func_unique_count}},

    {"03847778-bb9c-4e8c-96d5-0c10335c4f34",
    {"Unsharp mask",
     filter_category_e::enhance,
     filter_type_enum_e::unsharp_mask,
     filter_func_unsharp_mask}},

    {"eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03",
    {"Decimate",
     filter_category_e::reduce,
     filter_type_enum_e::decimate,
     filter_func_decimate}},

    {"94adffac-be42-43ac-9839-9cc53a6d615c",
    {"Denoise (temporal)",
     filter_category_e::enhance,
     filter_type_enum_e::denoise_temporal,
     filter_func_denoise_temporal}},

    {"e31d5ee3-f5df-4e7c-81b8-227fc39cbe76",
    {"Denoise (non-linear means)",
     filter_category_e::enhance,
     filter_type_enum_e::denoise_nonlocal_means,
     filter_func_denoise_nonlocal_means}},

    {"1c25bbb1-dbf4-4a03-93a1-adf24b311070",
    {"Sharpen",
     filter_category_e::enhance,
     filter_type_enum_e::sharpen,
     filter_func_sharpen}},

    {"de60017c-afe5-4e5e-99ca-aca5756da0e8",
    {"Median",
     filter_category_e::reduce,
     filter_type_enum_e::median,
     filter_func_median}},

    {"2448cf4a-112d-4d70-9fc1-b3e9176b6684",
    {"Crop",
     filter_category_e::distort,
     filter_type_enum_e::crop,
     filter_func_crop}},

    {"80a3ac29-fcec-4ae0-ad9e-bbd8667cc680",
    {"Flip",
     filter_category_e::distort,
     filter_type_enum_e::flip,
     filter_func_flip}},

    {"140c514d-a4b0-4882-abc6-b4e9e1ff4451",
    {"Rotate",
     filter_category_e::distort,
     filter_type_enum_e::rotate,
     filter_func_rotate}},

    {"136deb34-ac79-46b1-a09c-d57dcfaa84ad",
    {"Input gate",
     filter_category_e::meta,
     filter_type_enum_e::input_gate,
     nullptr}},

    {"be8443e2-4355-40fd-aded-63cebcbfb8ce",
    {"Output gate",
     filter_category_e::meta,
     filter_type_enum_e::output_gate,
     nullptr}},
};

// All filters the user has added to the filter graph.
static std::vector<filter_c*> FILTER_POOL;

// Sets of filters, starting with an input gate, followed by a number of filters,
// and ending with an output gate. These chains will be used to filter incoming
// frames.
static std::vector<std::vector<const filter_c*>> FILTER_CHAINS;

// The index in the list of filter chains of the chain that was most recently used.
// Generally, this will be the filter chain that matches the current input/output
// resolution.
static int MOST_RECENT_FILTER_CHAIN_IDX = -1;

std::string kf_filter_name_for_type(const filter_type_enum_e type)
{
    for (const auto filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType.second.type == type)
        {
            return filterType.second.name;
        }
    }

    k_assert(0, "Unable to find a name for the given filter type.");

    return "(unknown)";
}

std::string kf_filter_name_for_id(const std::string id)
{
    return kf_filter_name_for_type(kf_filter_type_for_id(id));
}

filter_type_enum_e kf_filter_type_for_id(const std::string id)
{
    return KNOWN_FILTER_TYPES.at(id).type;
}

std::string kf_filter_id_for_type(const filter_type_enum_e type)
{
    for (const auto filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType.second.type == type)
        {
            return filterType.first;
        }
    }

    k_assert(0, "Unable to find an id for the given filter type.");

    return "(unknown)";
}

// Apply to the given pixel buffer the chain of filters (if any) whose input gate
// matches the frame's resolution and output gate that of the current output resolution.
void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r)
{
    if (!FILTERING_ENABLED) return;

    k_assert((r.bpp == 32), "Filters can only be applied to 32-bit pixel data.");

    std::pair<const std::vector<const filter_c*>*, unsigned> partialMatch = {nullptr, 0};
    std::pair<const std::vector<const filter_c*>*, unsigned> openMatch = {nullptr, 0};
    const resolution_s outputRes = ks_output_resolution();

    static const auto apply_chain = [=](const std::vector<const filter_c*> &chain, const unsigned idx)
    {
        // The gate filters are expected to be #first and #last, while the actual
        // applicable filters are the ones in-between.
        for (unsigned c = 1; c < (chain.size() - 1); c++)
        {
            chain[c]->metaData.apply(pixels, &r, chain[c]->parameterData.ptr());
        }

        MOST_RECENT_FILTER_CHAIN_IDX = idx;

        return;
    };

    // Apply the first filter chain, if any, whose input and output resolution matches
    // those of the frame and the current scaler. If no such chain is found, we'll secondarily
    // apply a matching partially or fully open chain (a chain being open if its input or
    // output node's resolution contains one or more 0 values).
    for (unsigned i = 0; i < FILTER_CHAINS.size(); i++)
    {
        const auto &filterChain = FILTER_CHAINS[i];

        const unsigned inputGateWidth = *(u16*)&(filterChain.front()->parameterData[0]);
        const unsigned inputGateHeight = *(u16*)&(filterChain.front()->parameterData[2]);

        const unsigned outputGateWidth = *(u16*)&(filterChain.back()->parameterData[0]);
        const unsigned outputGateHeight = *(u16*)&(filterChain.back()->parameterData[2]);

        // A gate size of 0 in either dimension means pass all values. Otherwise, the
        // value must match the corresponding size of the frame or output.
        if (!inputGateWidth &&
            !inputGateHeight &&
            !outputGateWidth &&
            !outputGateHeight)
        {
            openMatch = {&filterChain, i};
        }
        else if ((!inputGateWidth || inputGateWidth == r.w) &&
                 (!inputGateHeight || inputGateHeight == r.h) &&
                 (!outputGateWidth || outputGateWidth == outputRes.w) &&
                 (!outputGateHeight || outputGateHeight == outputRes.h))
        {
            partialMatch = {&filterChain, i};
        }
        else if ((r.w == inputGateWidth) &&
                 (r.h == inputGateHeight) &&
                 (outputRes.w == outputGateWidth) &&
                 (outputRes.h == outputGateHeight))
        {
            apply_chain(filterChain, i);

            return;
        }
    }

    if (partialMatch.first)
    {
        apply_chain(*partialMatch.first, partialMatch.second);
    }
    else if (openMatch.first)
    {
        apply_chain(*openMatch.first, openMatch.second);
    }

    return;
}

std::vector<const filter_c::filter_metadata_s*> kf_known_filter_types(void)
{
    std::vector<const filter_c::filter_metadata_s*> filtersMetadata;

    for (const auto &filter: KNOWN_FILTER_TYPES)
    {
        filtersMetadata.push_back(&filter.second);
    }

    return filtersMetadata;
}

void kf_add_filter_chain(std::vector<const filter_c*> newChain)
{
    k_assert((newChain.size() >= 2) &&
             (newChain.at(0)->metaData.type == filter_type_enum_e::input_gate) &&
             (newChain.at(newChain.size()-1)->metaData.type == filter_type_enum_e::output_gate),
             "Detected a malformed filter chain.");

    FILTER_CHAINS.push_back(newChain);

    return;
}

void kf_remove_all_filter_chains(void)
{
    FILTER_CHAINS.clear();
    MOST_RECENT_FILTER_CHAIN_IDX = -1;

    return;
}

const filter_c* kf_create_new_filter_instance(const char *const id)
{
    filter_c *filter = new filter_c(id);

    FILTER_POOL.push_back(filter);

    return filter;
}

void kf_delete_filter_instance(const filter_c *const filter)
{
    const auto entry = std::find(FILTER_POOL.begin(), FILTER_POOL.end(), filter);

    if (entry != FILTER_POOL.end())
    {
        delete (*entry);
        FILTER_POOL.erase(entry);
    }

    return;
}

const filter_c* kf_create_new_filter_instance(const filter_type_enum_e type,
                                              const u8 *const initialParameterValues)
{
    filter_c *newFilterInstance = nullptr;

    for (const auto filter: KNOWN_FILTER_TYPES)
    {
        if (filter.second.type == type)
        {
            newFilterInstance = new filter_c(filter.first, initialParameterValues);
            break;
        }
    }

    k_assert(newFilterInstance, "Failed to create a filter of the given type.");

    FILTER_POOL.push_back(newFilterInstance);

    return newFilterInstance;
}

void kf_delete_filter(const filter_c *const filter)
{
    const auto filterEntry = std::find(FILTER_POOL.begin(), FILTER_POOL.end(), filter);

    k_assert(filterEntry != FILTER_POOL.end(),"Was asked to delete an unknown filter.");

    FILTER_POOL.erase(filterEntry);

    kd_refresh_filter_chains();

    return;
}

void kf_release_filters(void)
{
    INFO(("Releasing custom filtering."));

    MOST_RECENT_FILTER_CHAIN_IDX = -1;

    for (auto filter: FILTER_POOL)
    {
        delete filter;
    }

    return;
}

void kf_set_filtering_enabled(const bool enabled)
{
    FILTERING_ENABLED = enabled;

    return;
}

bool kf_is_filtering_enabled(void)
{
    return FILTERING_ENABLED;
}

// Find by how many pixels the given image is out of alignment with the
// edges of the capture screen vertically and horizontally, by counting how
// many vertical/horizontal columns/rows at the edges of the image contain
// nothing but black pixels. This is an estimate and not necessarily accurate -
// depending on the image, it may be grossly inaccurate. The vector returned
// contains as two components the image's alignment offset on the vertical
// and horizontal axes.
//
// This will not work if the image is fully contained within the capture screen -
// at least one edge of the image is expected to fall outside of the screen.
std::vector<int> kf_find_capture_alignment(u8 *const pixels, const resolution_s &r)
{
    k_assert((r.bpp == 32), "Expected 32-bit pixel data for finding image alignment.");

    // The level above which we consider a pixel's color value to be not part
    // of the black background.
    const int threshold = 50;

    const auto find_horizontal = [r, pixels, threshold](int x, int limit, int direction)->uint
    {
        uint steps = 0;
        while (x != limit)
        {
            for (uint y = 0; y < r.h; y++)
            {
                // If any pixel on this vertical column is above the threshold,
                // consider the column to be part of the image rather than of
                // the background.
                const uint idx = ((x + y * r.w) * 4);
                if (pixels[idx + 0] > threshold ||
                    pixels[idx + 1] > threshold ||
                    pixels[idx + 2] > threshold)
                {
                    goto done;
                }
            }

            steps++;
            x += direction;
        }

        done:
        return steps;
    };

    const auto find_vertical = [r, pixels, threshold](int y, int limit, int direction)->uint
    {
        uint steps = 0;
        while (y != limit)
        {
            for (uint x = 0; x < r.w; x++)
            {
                const uint idx = ((x + y * r.w) * 4);
                if (pixels[idx + 0] > threshold ||
                    pixels[idx + 1] > threshold ||
                    pixels[idx + 2] > threshold)
                {
                    goto done;
                }
            }

            steps++;
            y += direction;
        }

        done:
        return steps;
    };

    int left = find_horizontal(0, r.w-1, +1);
    int right = find_horizontal(r.w-1, 0, -1);
    int top = find_vertical(0, r.h-1, +1);
    int bottom = find_vertical(r.h-1, 0, -1);

    return {(left? left : -right),
            (top? top : -bottom)};
}

int kf_current_filter_chain_idx(void)
{
    return MOST_RECENT_FILTER_CHAIN_IDX;
}

void kf_initialize_filters(void)
{
    INFO(("Initializing custom filtering."));

    return;
}

filter_c::filter_c(const std::string &id, const u8 *initialParameterValues) :
    metaData(KNOWN_FILTER_TYPES.at(id)),
    parameterData(heap_bytes_s<u8>(FILTER_PARAMETER_ARRAY_LENGTH, "Filter parameter data")),
    guiWidget(create_gui_widget(initialParameterValues))
{
    return;
}

filter_c::~filter_c()
{
    delete this->guiWidget;
    this->parameterData.release_memory();

    return;
}

filter_widget_s *filter_c::create_gui_widget(const u8 *const initialParameterValues)
{
    u8 *const paramArray = this->parameterData.ptr();

    #define arguments paramArray, initialParameterValues

    switch (this->metaData.type)
    {
        case filter_type_enum_e::blur:                   return new filter_widget_blur_s(arguments);
        case filter_type_enum_e::rotate:                 return new filter_widget_rotate_s(arguments);
        case filter_type_enum_e::crop:                   return new filter_widget_crop_s(arguments);
        case filter_type_enum_e::flip:                   return new filter_widget_flip_s(arguments);
        case filter_type_enum_e::median:                 return new filter_widget_median_s(arguments);
        case filter_type_enum_e::denoise_temporal:       return new filter_widget_denoise_temporal_s(arguments);
        case filter_type_enum_e::denoise_nonlocal_means: return new filter_widget_denoise_nonlocal_means_s(arguments);
        case filter_type_enum_e::sharpen:                return new filter_widget_sharpen_s(arguments);
        case filter_type_enum_e::unsharp_mask:           return new filter_widget_unsharp_mask_s(arguments);
        case filter_type_enum_e::decimate:               return new filter_widget_decimate_s(arguments);
        case filter_type_enum_e::delta_histogram:        return new filter_widget_delta_histogram_s(arguments);
        case filter_type_enum_e::unique_count:           return new filter_widget_unique_count_s(arguments);

        case filter_type_enum_e::input_gate:             return new filter_widget_input_gate_s(arguments);
        case filter_type_enum_e::output_gate:            return new filter_widget_output_gate_s(arguments);

        default: k_assert(0, "No GUI widget is yet available for the given filter type.");
    }

    #undef arguments
}
