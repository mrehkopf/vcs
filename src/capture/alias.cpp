/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <vector>
#include "capture/capture.h"
#include "common/globals.h"
#include "capture/alias.h"

// A list of all the aliases we know of.
static std::vector<resolution_alias_s> ALIASES;

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

const std::vector<resolution_alias_s>& ka_aliases(void)
{
    return ALIASES;
}

void ka_set_aliases(const std::vector<resolution_alias_s> &aliases)
{
    ALIASES = aliases;

    if (!kc_has_signal())
    {
        return;
    }

    // If one of the aliases matches the current input resolution, change the
    // resolution accordingly.
    if (kc_has_signal())
    {
        const resolution_s currentRes = resolution_s::from_capture_device_properties();

        for (const auto &alias: ALIASES)
        {
            if (alias.from.w == currentRes.w &&
                alias.from.h == currentRes.h)
            {
                resolution_s::to_capture_device_properties(alias.to);
                break;
            }
        }
    }

    return;
}
