/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/median/filter_median.h"
#include "filter/filters/median/gui/filtergui_median.h"

filtergui_median_c::filtergui_median_c(abstract_filter_c *const filter)
{
    auto *const radius = new abstract_gui::spinner;
    radius->on_change = [=](const int value){filter->set_parameter(filter_median_c::PARAM_KERNEL_SIZE, ((value * 2) + 1));};
    radius->initialValue = (filter->parameter(filter_median_c::PARAM_KERNEL_SIZE) / 2);
    radius->minValue = 0;
    radius->maxValue = 99;
    this->fields.push_back({"Radius", {radius}});

    return;
}
