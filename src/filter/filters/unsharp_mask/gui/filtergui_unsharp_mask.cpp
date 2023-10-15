/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include "filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h"

filtergui_unsharp_mask_c::filtergui_unsharp_mask_c(abstract_filter_c *const filter)
{
    auto *strength = filtergui::spinner(filter, filter_unsharp_mask_c::PARAM_STRENGTH);
    strength->minValue = 0;
    strength->maxValue = 255;
    this->fields.push_back({"Strength", {strength}});

    auto *radius = filtergui::spinner(filter, filter_unsharp_mask_c::PARAM_RADIUS);
    radius->minValue = 1;
    radius->maxValue = 255;
    this->fields.push_back({"Radius", {radius}});

    return;
}
