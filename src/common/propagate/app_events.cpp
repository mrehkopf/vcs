/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/propagate/app_events.h"

static vcs_event_pool_c EVENTS;

vcs_event_pool_c& ke_events(void)
{
    return EVENTS;
}

void vcs_event_c::subscribe(vcs_event_handler_fn_t handlerFn)
{
    this->subscribedHandlers.push_back(handlerFn);

    return;
}

void vcs_event_c::fire(void) const
{
    for (const auto &handlerFn: this->subscribedHandlers)
    {
        handlerFn();
    }

    return;
}
