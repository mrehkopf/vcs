/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_GUI_FILTERGUI_DENOISE_PIXEL_GATE_H
#define VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_GUI_FILTERGUI_DENOISE_PIXEL_GATE_H

#include "common/abstract_gui.h"

class filtergui_denoise_pixel_gate_c : public abstract_gui_s
{
public:
    filtergui_denoise_pixel_gate_c(abstract_filter_c *const filter);
};

#endif
