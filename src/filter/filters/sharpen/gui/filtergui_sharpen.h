/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H
#define VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H

#include "common/abstract_gui.h"

class filtergui_sharpen_c : public abstract_gui_s
{
public:
    filtergui_sharpen_c(abstract_filter_c *const filter);
};

#endif
