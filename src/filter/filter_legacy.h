#ifndef FILTER_LEGACY_H_
#define FILTER_LEGACY_H_

#include <vector>
#include "common/globals.h"
#include "common/types.h"

struct scaling_filter_s;

// A single filter.
struct legacy14_filter_s
{
    // An internal string that uniquely identifies this filter.
    std::string uuid;

    // A name string that identifies this filter in the GUI.
    std::string name;

    // A set of bytes you can cast into the filter's parameters.
    u8 data[FILTER_PARAMETER_ARRAY_LENGTH];
};

// A set of filters (pre/post-scaling) for a particular input/output resolution activation.
struct legacy14_filter_set_s
{
    // A user-facing, user-specified string that describes in brief this filter's
    // purpose or the like.
    std::string description = "";

    const scaling_filter_s *scaler = nullptr;     // The scaler used for up/downscaling.

    std::vector<legacy14_filter_s> preFilters;   // Filters applied before scaling.
    std::vector<legacy14_filter_s> postFilters;  // Filters applied after scaling.

    // Whether this filter set activates for all resolutions, a given input resolution,
    // or a given combination of input and output resolutions.
    enum activation_e : uint { none = 0, all = 1, in = 2, out = 4 };
    uint activation = activation_e::none;

    // The resolution(s) in which this filter ser operates, dependent on its
    // activation type.
    resolution_s inRes, outRes;

    bool isEnabled = true;
};

#endif
