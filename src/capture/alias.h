#ifndef CAPTURE_ALIAS_H
#define CAPTURE_ALIAS_H

#include "display/display.h"

struct mode_alias_s
{
    resolution_s from;
    resolution_s to;
};

resolution_s ka_aliased(const resolution_s &r);

int ka_index_of_alias_resolution(const resolution_s r);
const std::vector<mode_alias_s>& ka_aliases(void);

void ka_set_aliases(const std::vector<mode_alias_s> &aliases);
void ka_broadcast_aliases_to_gui(void);

#endif
