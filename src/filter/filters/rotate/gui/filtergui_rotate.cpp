/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/rotate/filter_rotate.h"
#include "filter/filters/rotate/gui/filtergui_rotate.h"

filtergui_rotate_c::filtergui_rotate_c(abstract_filter_c *const filter)
{
    {
        auto *const angle = new abstract_gui::double_spinner;

        angle->get_value = [=]{return filter->parameter(filter_rotate_c::PARAM_ROT);};
        angle->set_value = [=](const double value){filter->set_parameter(filter_rotate_c::PARAM_ROT, value);};
        angle->minValue = -360;
        angle->maxValue = 360;
        angle->numDecimals = 2;

        this->fields.push_back({"Angle", {angle}});
    }

    {
        auto *const scale = new abstract_gui::double_spinner;

        scale->get_value = [=]{return filter->parameter(filter_rotate_c::PARAM_SCALE);};
        scale->set_value = [=](const double value){filter->set_parameter(filter_rotate_c::PARAM_SCALE, value);};
        scale->minValue = 0.01;
        scale->maxValue = 20;
        scale->numDecimals = 2;
        scale->stepSize = 0.1;

        this->fields.push_back({"Scale", {scale}});
    }

    return;
}
