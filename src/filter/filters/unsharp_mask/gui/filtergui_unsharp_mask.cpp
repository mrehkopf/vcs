/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include "filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h"

filtergui_unsharp_mask_c::filtergui_unsharp_mask_c(abstract_filter_c *const filter)
{
    {
        auto *const strength = new filtergui_spinbox_s;

        strength->get_value = [=]{return filter->parameter(filter_unsharp_mask_c::PARAM_STRENGTH);};
        strength->set_value = [=](const double value){filter->set_parameter(filter_unsharp_mask_c::PARAM_STRENGTH, value);};
        strength->minValue = 0;
        strength->maxValue = 255;

        this->guiFields.push_back({"Strength", {strength}});
    }

    {
        auto *const radius = new filtergui_spinbox_s;

        radius->get_value = [=]{return filter->parameter(filter_unsharp_mask_c::PARAM_RADIUS);};
        radius->set_value = [=](const double value){filter->set_parameter(filter_unsharp_mask_c::PARAM_RADIUS, value);};
        radius->minValue = 1;
        radius->maxValue = 255;

        this->guiFields.push_back({"Radius", {radius}});
    }

    return;
}
