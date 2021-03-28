/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_GUI_FILTERGUI_DENOISE_PIXEL_GATE_H
#define VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_GUI_FILTERGUI_DENOISE_PIXEL_GATE_H

#include "filter/filters/qt_filtergui.h"

class filtergui_denoise_pixel_gate_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_denoise_pixel_gate_c(filter_c *const filter);
};

#endif
