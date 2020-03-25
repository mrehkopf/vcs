/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <vector>
#include "capture/video_parameters.h"
#include "capture/capture_api.h"
#include "capture/capture.h"

// One set of parameters for a given capture resolution for which such
// parameters are known.
static std::vector<video_signal_parameters_s> KNOWN_PARAMETER_SETS;

const std::vector<video_signal_parameters_s>& kvideoparam_all_parameter_sets(void)
{
    return KNOWN_PARAMETER_SETS;
}

video_signal_parameters_s kvideoparam_parameters_for_resolution(const resolution_s r)
{
    for (const auto &m: KNOWN_PARAMETER_SETS)
    {
        if (m.r.w == r.w &&
            m.r.h == r.h)
        {
            return m;
        }
    }

    // If we don't have parameters for the given resolution, we'll use the
    // capture device's defaults.
    auto defaultParams = kc_capture_api().get_default_video_signal_parameters();
    defaultParams.r = r;

    return defaultParams;
}

void kvideoparam_assign_parameter_sets(const std::vector<video_signal_parameters_s> &paramSets)
{
    KNOWN_PARAMETER_SETS = paramSets;

    return;
}


void kvideoparam_update_parameters_for_resolution(const resolution_s r,
                                                  const video_signal_parameters_s &newParams)
{
    unsigned idx = 0;

    for (idx = 0; idx < KNOWN_PARAMETER_SETS.size(); idx++)
    {
        if (KNOWN_PARAMETER_SETS[idx].r.w == r.w &&
            KNOWN_PARAMETER_SETS[idx].r.h == r.h)
        {
            goto mode_exists;
        }
    }

    // If we didn't already know of any parameters for this resolution, add
    // them.
    {
        auto defaultParams = kc_capture_api().get_default_video_signal_parameters();
        defaultParams.r = r;

        KNOWN_PARAMETER_SETS.push_back(defaultParams);
    }

    mode_exists:
    KNOWN_PARAMETER_SETS[idx] = newParams;

    return;
}
