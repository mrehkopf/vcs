/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h"

filtergui_denoise_pixel_gate_c::filtergui_denoise_pixel_gate_c(abstract_filter_c *const filter)
{
    auto *strength = filtergui::spinner(filter, filter_denoise_pixel_gate_c::PARAM_STRENGTH);
    strength->minValue = 0;
    strength->maxValue = 255;
    this->fields.push_back({"Strength", {strength}});

    return;
}
