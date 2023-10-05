/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/crt/filter_crt.h"
#include "filter/filters/crt/gui/filtergui_crt.h"

filtergui_crt_c::filtergui_crt_c(abstract_filter_c *const filter)
{
    {
        auto *const blurType = new abstract_gui::combo_box;

        blurType->get_value = [=]{return filter->parameter(filter_crt_c::PARAM_CRT_TYPE);};
        blurType->set_value = [=](const int value){filter->set_parameter(filter_crt_c::PARAM_CRT_TYPE, std::max(0, value));};
        blurType->items = {"Crap"};

        this->fields.push_back({"Screen quality", {blurType}});
    }

    return;
}
