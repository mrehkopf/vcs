    /*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <cstring>
#include <vector>
#include <cmath>
#include <map>
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
static std::vector<std::vector<const filter_c*>> FILTER_CHAINS;

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
        new filter_input_gate_c(),
        new filter_output_gate_c(),
    };

    return;
}

filter_type_e kf_filter_type_for_id(const std::string id)
{
    for (const auto *filter: KNOWN_FILTER_TYPES)
    {
        if (filter->uuid() == id)
        {
            return filter->type();
        }
    }

    return filter_type_e::unknown;
}

// Apply to the given pixel buffer the chain of filters (if any) whose input gate
// matches the frame's resolution and output gate that of the current output resolution.
void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r)
{
    if (!FILTERING_ENABLED) return;

    k_assert((r.bpp == 32), "Filters can only be applied to 32-bit pixel data.");

    std::pair<const std::vector<const filter_c*>*, unsigned> partialMatch = {nullptr, 0};
    std::pair<const std::vector<const filter_c*>*, unsigned> openMatch = {nullptr, 0};
    const resolution_s outputRes = ks_scaler_output_resolution();

    static const auto apply_chain = [&pixels, &r](const std::vector<const filter_c*> &chain, const unsigned idx)
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

const std::vector<const filter_c*>& kf_known_filter_types(void)
{
    return KNOWN_FILTER_TYPES;
}

void kf_add_filter_chain(std::vector<const filter_c*> newChain)
{
    k_assert((newChain.size() >= 2) &&
             (newChain.at(0)->type() == filter_type_e::input_gate) &&
             (newChain.at(newChain.size()-1)->type() == filter_type_e::output_gate),
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

const filter_c* kf_create_new_filter_instance(const filter_type_e type,
                                                const u8 *const initialParameterValues)
{
    filter_c *filter = nullptr;

    switch (type)
    {
        case filter_type_e::blur:                   filter = new filter_blur_c(initialParameterValues); break;
        case filter_type_e::delta_histogram:        filter = new filter_delta_histogram_c(initialParameterValues); break;
        case filter_type_e::frame_rate:             filter = new filter_frame_rate_c(initialParameterValues); break;
        case filter_type_e::unsharp_mask:           filter = new filter_unsharp_mask_c(initialParameterValues); break;
        case filter_type_e::decimate:               filter = new filter_decimate_c(initialParameterValues); break;
        case filter_type_e::denoise_pixel_gate:     filter = new filter_denoise_pixel_gate_c(initialParameterValues); break;
        case filter_type_e::denoise_nonlocal_means: filter = new filter_denoise_nonlocal_means_c(initialParameterValues); break;
        case filter_type_e::sharpen:                filter = new filter_sharpen_c(initialParameterValues); break;
        case filter_type_e::median:                 filter = new filter_median_c(initialParameterValues); break;
        case filter_type_e::crop:                   filter = new filter_crop_c(initialParameterValues); break;
        case filter_type_e::flip:                   filter = new filter_flip_c(initialParameterValues); break;
        case filter_type_e::rotate:                 filter = new filter_rotate_c(initialParameterValues); break;

        case filter_type_e::input_gate:             filter = new filter_input_gate_c(initialParameterValues); break;
        case filter_type_e::output_gate:            filter = new filter_output_gate_c(initialParameterValues); break;

        case filter_type_e::unknown:                filter = nullptr; break;
    }

    FILTER_POOL.push_back(filter);

    return filter;
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

filter_c::filter_c(const u8 *const initialParameterArray,
                   const std::vector<filter_c::parameter_s> &parameters)
{
    this->parameterData.alloc(FILTER_PARAMETER_ARRAY_LENGTH);

    if (initialParameterArray)
    {
        std::memcpy(this->parameterData.ptr(),
                    initialParameterArray,
                    this->parameterData.up_to(FILTER_PARAMETER_ARRAY_LENGTH));
    }
    else
    {
        memset(this->parameterData.ptr(),
               0,
               this->parameterData.up_to(FILTER_PARAMETER_ARRAY_LENGTH));
    }

    for (const auto &parameter: parameters)
    {
        k_assert((this->parameters.find(parameter.offset) == this->parameters.end()),
                 "Duplicate parameter offset.");

        this->parameters[parameter.offset] = parameter;

        if (!initialParameterArray)
        {
            this->set_parameter(parameter.offset, parameter.defaultValue);
        }
    }

    return;
}

filter_c::~filter_c(void)
{
    this->parameterData.release_memory();

    delete this->guiWidget;

    return;
}
