/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_GUI_FILTERGUI_KERNEL_3X3_H
#define VCS_FILTER_FILTERS_SHARPEN_GUI_FILTERGUI_KERNEL_3X3_H

#include "filter/filtergui.h"

class filtergui_kernel_3x3_c : public filtergui_c
{
public:
    filtergui_kernel_3x3_c(abstract_filter_c *const filter);
};

#endif
