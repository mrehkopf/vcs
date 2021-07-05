/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/flip/filter_flip.h"
#include "filter/filters/flip/gui/filtergui_flip.h"

filtergui_flip_c::filtergui_flip_c(abstract_filter_c *const filter)
{
    {
        auto *const axis = new filtergui_combobox_s;

        axis->get_value = [=]{return filter->parameter(filter_flip_c::PARAM_AXIS);};
        axis->set_value = [=](const double value){filter->set_parameter(filter_flip_c::PARAM_AXIS, value);};
        axis->items = {"Vertical", "Horizontal", "Both"};

        this->guiFields.push_back({"Axis", {axis}});
    }

    return;
}
