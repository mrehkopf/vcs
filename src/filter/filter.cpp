/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <algorithm>
#include <functional>
#include <cstring>
#include <vector>
#include <cmath>
#include "display/display.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "filter/filters/filters.h"

// Whether filters (if any are activated) should be applied to incoming frames.
static bool FILTERING_ENABLED = false;

// This will contain a list of the filter types available to the program.
static std::vector<const abstract_filter_c*> KNOWN_FILTER_TYPES;

// All filters the user has added to the filter graph.
static std::vector<abstract_filter_c*> FILTER_POOL;

// Sets of filters, starting with an input gate, followed by a number of filters,
// and ending with an output gate. These chains will be used to filter incoming
// frames.
static std::vector<std::vector<abstract_filter_c*>> FILTER_CHAINS;

// The index in the list of filter chains of the chain that was most recently used.
// Generally, this will be the filter chain that matches the current input/output
// resolution.
static int MOST_RECENT_FILTER_CHAIN_IDX = -1;

void kf_initialize_filters(void)
{
    INFO(("Initializing the filter subsystem."));

    KNOWN_FILTER_TYPES =
    {
        new filter_blur_c(),
        new filter_delta_histogram_c(),
        new filter_frame_rate_c(),
        new filter_unsharp_mask_c(),
        new filter_decimate_c(),
        new filter_denoise_pixel_gate_c(),
        new filter_denoise_nonlocal_means_c(),
        new filter_sharpen_c(),
        new filter_median_c(),
        new filter_crop_c(),
        new filter_flip_c(),
        new filter_rotate_c(),
        new filter_kernel_3x3_c(),
        new filter_color_depth_c(),
        new filter_anti_tear_c(),
        new filter_input_gate_c(),
        new filter_output_gate_c(),
    };

    for (unsigned i = 0; i < KNOWN_FILTER_TYPES.size(); i++)
    {
        for (unsigned c = (i + 1); c < KNOWN_FILTER_TYPES.size(); c++)
        {
            k_assert((KNOWN_FILTER_TYPES.at(i)->uuid() != KNOWN_FILTER_TYPES.at(c)->uuid()),
                     "Duplicate filter UUIDs detected.");
        }
    }

    return;
}

// Apply to the given pixel buffer the chain of filters (if any) whose input gate
// matches the frame's resolution and output gate that of the current output resolution.
void kf_apply_matching_filter_chain(u8 *const pixels, const resolution_s &r)
{
    if (!FILTERING_ENABLED) return;

    k_assert((r.bpp == 32), "Filters can only be applied to 32-bit pixel data.");

    std::pair<const std::vector<abstract_filter_c*>*, unsigned> partialMatch = {nullptr, 0};
    std::pair<const std::vector<abstract_filter_c*>*, unsigned> openMatch = {nullptr, 0};
    const resolution_s outputRes = ks_output_resolution();

    static const auto apply_chain = [&pixels, &r](const std::vector<abstract_filter_c*> &chain, const unsigned idx)
    {
        // The gate filters are expected to be #first and #last, while the actual
        // applicable filters are the ones in-between.
        for (unsigned c = 1; c < (chain.size() - 1); c++)
        {
            chain[c]->apply(pixels, r);
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

        const unsigned inputGateWidth = filterChain.front()->parameter(filter_input_gate_c::PARAM_WIDTH);
        const unsigned inputGateHeight = filterChain.front()->parameter(filter_input_gate_c::PARAM_HEIGHT);

        const unsigned outputGateWidth = filterChain.back()->parameter(filter_output_gate_c::PARAM_WIDTH);
        const unsigned outputGateHeight = filterChain.back()->parameter(filter_output_gate_c::PARAM_HEIGHT);

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

const std::vector<const abstract_filter_c*>& kf_available_filter_types(void)
{
    return KNOWN_FILTER_TYPES;
}

void kf_register_filter_chain(std::vector<abstract_filter_c*> newChain)
{
    k_assert((newChain.size() >= 2) &&
             (newChain.at(0)->category() == filter_category_e::input_condition) &&
             (newChain.at(newChain.size()-1)->category() == filter_category_e::output_condition),
             "Detected a malformed filter chain.");

    FILTER_CHAINS.push_back(newChain);

    return;
}

void kf_unregister_all_filter_chains(void)
{
    FILTER_CHAINS.clear();
    MOST_RECENT_FILTER_CHAIN_IDX = -1;

    return;
}

void kf_delete_filter_instance(const abstract_filter_c *const filter)
{
    const auto entry = std::find(FILTER_POOL.begin(), FILTER_POOL.end(), filter);

    if (entry != FILTER_POOL.end())
    {
        delete (*entry);
        FILTER_POOL.erase(entry);
    }

    return;
}

abstract_filter_c* kf_create_filter_instance(const std::string &filterTypeUuid,
                                             const std::vector<std::pair<unsigned, double>> &initialParams)
{
    abstract_filter_c *filter = nullptr;

    for (auto &filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType->uuid() == filterTypeUuid)
        {
            filter = filterType->create_clone();
            filter->set_parameters(initialParams);
            break;
        }
    }

    k_assert(filter, "Unknown filter type.");

    FILTER_POOL.push_back(filter);

    return filter;
}

bool kf_is_known_filter_uuid(const std::string &filterTypeUuid)
{
    for (auto &filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType->uuid() == filterTypeUuid)
        {
            return true;
        }
    }

    return false;
}

void kf_release_filters(void)
{
    INFO(("Releasing custom filtering."));

    MOST_RECENT_FILTER_CHAIN_IDX = -1;

    for (auto *filter: FILTER_POOL)
    {
        delete filter;
    }

    for (const auto *filter: KNOWN_FILTER_TYPES)
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

int kf_current_filter_chain_idx(void)
{
    return MOST_RECENT_FILTER_CHAIN_IDX;
}
