/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/median/filter_median.h"
#include "filter/filters/median/gui/filtergui_median.h"

filtergui_median_c::filtergui_median_c(filter_c *const filter)
{
    {
        auto *const radius = new filtergui_spinbox_s;

        radius->get_value = [=]{return (filter->parameter(filter_median_c::PARAM_KERNEL_SIZE) / 2);};
        radius->set_value = [=](const double value){filter->set_parameter(filter_median_c::PARAM_KERNEL_SIZE, ((value * 2) + 1));};
        radius->minValue = 0;
        radius->maxValue = 99;

        this->guiFields.push_back({"Radius", {radius}});
    }

    return;
}
