/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H
#define VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H

#include "filter/filtergui.h"

class filtergui_sharpen_c : public filtergui_c
{
public:
    filtergui_sharpen_c(abstract_filter_c *const filter);
};

#endif
