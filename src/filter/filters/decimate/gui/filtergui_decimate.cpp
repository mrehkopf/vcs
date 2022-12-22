/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/decimate/filter_decimate.h"
#include "filter/filters/decimate/gui/filtergui_decimate.h"

filtergui_decimate_c::filtergui_decimate_c(abstract_filter_c *const filter)
{
    {
        auto *const factor = new filtergui_spinbox_s;

        factor->get_value = [=]{return filter->parameter(filter_decimate_c::PARAM_FACTOR);};
        factor->set_value = [=](const int value){filter->set_parameter(filter_decimate_c::PARAM_FACTOR, std::pow(2, value));};
        factor->minValue = 1;
        factor->maxValue = 4;

        this->guiFields.push_back({"Factor", {factor}});
    }

    {
        auto *const sampler = new filtergui_combobox_s;

        sampler->get_value = [=]{return filter->parameter(filter_decimate_c::PARAM_TYPE);};
        sampler->set_value = [=](const int value){filter->set_parameter(filter_decimate_c::PARAM_TYPE, value);};
        sampler->items = {"Nearest", "Average"};

        this->guiFields.push_back({"Sampler", {sampler}});
    }

    return;
}
