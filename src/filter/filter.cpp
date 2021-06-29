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
#include "filter/filters/filters.h"

// Whether filters (if any are activated) should be applied to incoming frames.
static bool FILTERING_ENABLED = false;

// This will contain a list of the filter types available to the program.
static std::vector<const filter_c*> KNOWN_FILTER_TYPES;

// All filters the user has added to the filter graph.
static std::vector<filter_c*> FILTER_POOL;

// Sets of filters, starting with an input gate, followed by a number of filters,
// and ending with an output gate. These chains will be used to filter incoming
// frames.
static std::vector<std::vector<filter_c*>> FILTER_CHAINS;

// The index in the list of filter chains of the chain that was most recently used.
// Generally, this will be the filter chain that matches the current input/output
// resolution.
static int MOST_RECENT_FILTER_CHAIN_IDX = -1;

void kf_initialize_filters(void)
{
    INFO(("Initializing custom filtering."));

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
void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r)
{
    if (!FILTERING_ENABLED) return;

    k_assert((r.bpp == 32), "Filters can only be applied to 32-bit pixel data.");

    std::pair<const std::vector<filter_c*>*, unsigned> partialMatch = {nullptr, 0};
    std::pair<const std::vector<filter_c*>*, unsigned> openMatch = {nullptr, 0};
    const resolution_s outputRes = ks_output_resolution();

    static const auto apply_chain = [&pixels, &r](const std::vector<filter_c*> &chain, const unsigned idx)
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

const std::vector<const filter_c*>& kf_known_filter_types(void)
{
    return KNOWN_FILTER_TYPES;
}

void kf_add_filter_chain(std::vector<filter_c*> newChain)
{
    k_assert((newChain.size() >= 2) &&
             (newChain.at(0)->category() == filter_category_e::gate_input) &&
             (newChain.at(newChain.size()-1)->category() == filter_category_e::gate_output),
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

filter_c* kf_create_new_filter_instance(const std::string &filterTypeUuid,
                                        const std::vector<std::pair<unsigned, double>> &initialParameterValues)
{
    filter_c *filter = nullptr;

    for (auto &filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType->uuid() == filterTypeUuid)
        {
            filter = filterType->create_clone();
            filter->set_parameters(initialParameterValues);
            break;
        }
    }

    k_assert(filter, "Unknown filter type.");

    FILTER_POOL.push_back(filter);

    return filter;
}

bool kf_is_known_filter_type(const std::string &filterTypeUuid)
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

filter_c::filter_c(const std::vector<std::pair<unsigned, double>> &parameters,
                   const std::vector<std::pair<unsigned, double>> &overrideParameterValues)
{
    this->parameterValues.resize(parameters.size());

    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    for (const auto &parameter: overrideParameterValues)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

filter_c::~filter_c(void)
{
    delete this->guiDescription;

    return;
}

const std::vector<filtergui_field_s>& filter_c::gui_description(void) const
{
    return this->guiDescription->guiFields;
}

unsigned filter_c::num_parameters(void) const
{
    return this->parameterValues.size();
}

double filter_c::parameter(const unsigned offset) const
{
    return this->parameterValues.at(offset);
}

void filter_c::set_parameter(const unsigned offset, const double value)
{
    if (offset < this->parameterValues.size())
    {
        this->parameterValues.at(offset) = value;
    }

    return;
}

void filter_c::set_parameters(const std::vector<std::pair<unsigned, double>> &parameters)
{
    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

std::vector<std::pair<unsigned, double>> filter_c::parameters(void) const
{
    auto params = std::vector<std::pair<unsigned, double>>{};

    for (unsigned i = 0; i < this->parameterValues.size(); i++)
    {
        params.push_back({i, this->parameterValues[i]});
    }

    return params;
}
