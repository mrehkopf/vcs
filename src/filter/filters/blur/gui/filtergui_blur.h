/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H
#define VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H

#include "common/abstract_gui.h"

class filtergui_blur_c : public abstract_gui_s
{
public:
    filtergui_blur_c(abstract_filter_c *const filter);
};

#endif
