/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/blur/filter_blur.h"
#include "filter/filters/blur/gui/filtergui_blur.h"

filtergui_blur_c::filtergui_blur_c(abstract_filter_c *const filter)
{
    {
        auto *const blurType = new filtergui_combobox_s;

        blurType->get_value = [=]{return filter->parameter(filter_blur_c::PARAM_TYPE);};
        blurType->set_value = [=](const int value){filter->set_parameter(filter_blur_c::PARAM_TYPE, std::max(0, value));};
        blurType->items = {"Box", "Gaussian"};

        this->guiFields.push_back({"Type", {blurType}});
    }

    {
        auto *const blurRadius = new filtergui_doublespinbox_s;

        blurRadius->get_value = [=]{return filter->parameter(filter_blur_c::PARAM_KERNEL_SIZE);};
        blurRadius->set_value = [=](const double value){filter->set_parameter(filter_blur_c::PARAM_KERNEL_SIZE, value);};
        blurRadius->minValue = 0.1;
        blurRadius->maxValue = 99;
        blurRadius->numDecimals = 2;

        this->guiFields.push_back({"Radius", {blurRadius}});
    }

    return;
}
