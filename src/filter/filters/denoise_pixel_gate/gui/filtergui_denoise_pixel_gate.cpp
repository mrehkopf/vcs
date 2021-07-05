/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h"

filtergui_denoise_pixel_gate_c::filtergui_denoise_pixel_gate_c(abstract_filter_c *const filter)
{
    {
        auto *const threshold = new filtergui_spinbox_s;

        threshold->get_value = [=]{return filter->parameter(filter_denoise_pixel_gate_c::PARAM_THRESHOLD);};
        threshold->set_value = [=](const double value){filter->set_parameter(filter_denoise_pixel_gate_c::PARAM_THRESHOLD, value);};
        threshold->minValue = 0;
        threshold->maxValue = 255;

        this->guiFields.push_back({"Threshold", {threshold}});
    }

    return;
}
