/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/color_depth/filter_color_depth.h"
#include "filter/filters/color_depth/gui/filtergui_color_depth.h"

filtergui_color_depth_c::filtergui_color_depth_c(abstract_filter_c *const filter)
{
    {
        auto *const red = new filtergui_spinbox_s;
        red->get_value = [=]{return filter->parameter(filter_color_depth_c::PARAM_BIT_COUNT_RED);};
        red->set_value = [=](const double value){filter->set_parameter(filter_color_depth_c::PARAM_BIT_COUNT_RED, value);};
        red->minValue = 1;
        red->maxValue = 8;

        auto *const green = new filtergui_spinbox_s;
        green->get_value = [=]{return filter->parameter(filter_color_depth_c::PARAM_BIT_COUNT_GREEN);};
        green->set_value = [=](const double value){filter->set_parameter(filter_color_depth_c::PARAM_BIT_COUNT_GREEN, value);};
        green->minValue = 1;
        green->maxValue = 8;

        auto *const blue = new filtergui_spinbox_s;
        blue->get_value = [=]{return filter->parameter(filter_color_depth_c::PARAM_BIT_COUNT_BLUE);};
        blue->set_value = [=](const double value){filter->set_parameter(filter_color_depth_c::PARAM_BIT_COUNT_BLUE, value);};
        blue->minValue = 1;
        blue->maxValue = 8;

        this->guiFields.push_back({"RGB", {red, green, blue}});
    }

    return;
}
