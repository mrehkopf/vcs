/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_GUI_FILTERGUI_DENOISE_NONLOCAL_MEANS_H
#define VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_GUI_FILTERGUI_DENOISE_NONLOCAL_MEANS_H

#include "common/abstract_gui.h"

class filtergui_denoise_nonlocal_means_c : public abstract_gui_s
{
public:
    filtergui_denoise_nonlocal_means_c(abstract_filter_c *const filter);
};

#endif
