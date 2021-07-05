/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H
#define VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H

#include "filter/filtergui.h"

class filtergui_blur_c : public filtergui_c
{
public:
    filtergui_blur_c(abstract_filter_c *const filter);
};

#endif
