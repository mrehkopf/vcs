/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <vector>
#include "common/propagate/propagate.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "capture/alias.h"

// A list of all the aliases we know of.
static std::vector<mode_alias_s> ALIASES;

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

void ka_broadcast_aliases_to_gui(void)
{
    DEBUG(("Broadcasting %u alias(es) to the GUI.", ALIASES.size()));

    kd_clear_aliases();
    for (const auto &a: ALIASES)
    {
        kd_add_alias(a);
    }

    return;
}

const std::vector<mode_alias_s>& ka_aliases(void)
{
    return ALIASES;
}

void ka_set_aliases(const std::vector<mode_alias_s> &aliases)
{
    ALIASES = aliases;

    if (kc_capture_api().has_no_signal())
    {
        return;
    }
    else
    {
        // If one of the aliases matches the current input resolution, change the
        // resolution accordingly.
        const resolution_s currentRes = kc_capture_api().get_resolution();
        for (const auto &alias: ALIASES)
        {
            if (alias.from.w == currentRes.w &&
                alias.from.h == currentRes.h)
            {
                kpropagate_forced_capture_resolution(alias.to);
                break;
            }
        }
    }

    return;
}
