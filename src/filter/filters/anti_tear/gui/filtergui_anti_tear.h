/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ANTI_TEAR_GUI_FILTERGUI_ANTI_TEAR_H
#define VCS_FILTER_FILTERS_ANTI_TEAR_GUI_FILTERGUI_ANTI_TEAR_H

#include "filter/filtergui.h"

class filtergui_anti_tear_c : public filtergui_c
{
public:
    filtergui_anti_tear_c(abstract_filter_c *const filter);
};

#endif
