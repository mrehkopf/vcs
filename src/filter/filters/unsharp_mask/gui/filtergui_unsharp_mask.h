/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_UNSHARP_MASK_GUI_FILTERGUI_UNSHARP_MASK_H
#define VCS_FILTER_FILTERS_UNSHARP_MASK_GUI_FILTERGUI_UNSHARP_MASK_H

#include "common/abstract_gui.h"

class filtergui_unsharp_mask_c : public abstract_gui_s
{
public:
    filtergui_unsharp_mask_c(abstract_filter_c *const filter);
};

#endif
