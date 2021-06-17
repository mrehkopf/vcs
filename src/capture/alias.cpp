/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <vector>
#include "common/propagate/app_events.h"
#include "capture/capture_device.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "capture/alias.h"

// A list of all the aliases we know of.
static std::vector<mode_alias_s> ALIASES;

void ka_initialize_aliases(void)
{
    return;
}

// Returns the index in the list of known aliases the alias whose source
// resolution matches the given resolution; or -1 is no such alias is
// found.
static int alias_index_of_source_resolution(const resolution_s &r)
{
    for (size_t i = 0; i < ALIASES.size(); i++)
    {
        if (ALIASES[i].from.w == r.w &&
            ALIASES[i].from.h == r.h)
        {
            return i;
        }
    }

    // No alias found.
    return -1;
}

bool ka_has_alias(const resolution_s &r)
{
    // Note: If there's no alias for the given resolution, this function
    // returns the same resolution.
    const resolution_s alias = ka_aliased(r);

    return bool((r.w != alias.w) || (r.h != alias.h));
}

resolution_s ka_aliased(const resolution_s &r)
{
    const int aliasIdx = alias_index_of_source_resolution(r);

    if (aliasIdx >= 0)
    {
        return ALIASES.at(aliasIdx).to;
    }

    return r;
}

const std::vector<mode_alias_s>& ka_aliases(void)
{
    return ALIASES;
}

void ka_set_aliases(const std::vector<mode_alias_s> &aliases)
{
    ALIASES = aliases;

    if (kc_capture_device().has_no_signal())
    {
        return;
    }
    else
    {
        // If one of the aliases matches the current input resolution, change the
        // resolution accordingly.
        const resolution_s currentRes = kc_capture_device().get_resolution();
        for (const auto &alias: ALIASES)
        {
            if (alias.from.w == currentRes.w &&
                alias.from.h == currentRes.h)
            {
                kc_force_input_resolution(alias.to);
                break;
            }
        }
    }

    return;
}
