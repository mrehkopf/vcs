/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CROP_GUI_FILTERGUI_CROP_H
#define VCS_FILTER_FILTERS_CROP_GUI_FILTERGUI_CROP_H

#include "common/abstract_gui.h"

class filtergui_crop_c : public abstract_gui_s
{
public:
    filtergui_crop_c(abstract_filter_c *const filter);
};

#endif
