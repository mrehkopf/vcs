/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DECIMATE_GUI_FILTERGUI_DECIMATE_H
#define VCS_FILTER_FILTERS_DECIMATE_GUI_FILTERGUI_DECIMATE_H

#include "filter/filtergui.h"

class filtergui_decimate_c : public filtergui_c
{
public:
    filtergui_decimate_c(abstract_filter_c *const filter);
};

#endif
