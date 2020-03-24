/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS capture aliases
 *
 * An alias is a resolution pair, 'from' and 'to', where any time the capture hardware
 * initializes the capture to the 'from' input resolution, VCS should tell the hardware
 * to set 'to' as the resolution, instead. This is useful in certain cases where the
 * hardware is consistently initializing the wrong input resolution and you want to
 * automate the process of correcting it.
 *
 */

#include <vector>
#include "common/propagate.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "capture/alias.h"

static std::vector<mode_alias_s> ALIASES;

int ka_index_of_alias_resolution(const resolution_s r)
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

// See if there isn't an alias resolution for the given resolution.
// If there is, will return that. Otherwise, returns the resolution
// that was passed in.
//
resolution_s ka_aliased(const resolution_s &r)
{
    const int aliasIdx = ka_index_of_alias_resolution(r);
    if (aliasIdx >= 0) return ALIASES.at(aliasIdx).to;

    return r;
}

// Lets the gui know which aliases we've got loaded.
void ka_broadcast_aliases_to_gui(void)
{
    DEBUG(("Broadcasting %u alias set(s) to the GUI.", ALIASES.size()));

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

    if (!kc_capture_api().no_signal())
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
