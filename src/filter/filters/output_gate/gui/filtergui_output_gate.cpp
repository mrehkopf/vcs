/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/output_gate/filter_output_gate.h"
#include "filter/filters/output_gate/gui/filtergui_output_gate.h"
#include "common/globals.h"

filtergui_output_gate_c::filtergui_output_gate_c(abstract_filter_c *const filter)
{
    {
        auto *const width = new abstract_gui::spinner;
        width->isInitiallyEnabled = filter->parameter(filter_output_gate_c::PARAM_IS_WIDTH_ENABLED);
        width->get_value = [=]{return filter->parameter(filter_output_gate_c::PARAM_WIDTH);};
        width->set_value = [=](const int value){filter->set_parameter(filter_output_gate_c::PARAM_WIDTH, value);};
        width->minValue = 0;
        width->maxValue = MAX_CAPTURE_WIDTH;

        auto *const toggle = new abstract_gui::checkbox;
        toggle->get_value = [=]{return filter->parameter(filter_output_gate_c::PARAM_IS_WIDTH_ENABLED);};
        toggle->set_value = [=](const bool value){
            filter->set_parameter(filter_output_gate_c::PARAM_IS_WIDTH_ENABLED, value);
            width->set_enabled(value);
        };

        this->fields.push_back({"X", {toggle, width}});
    }

    {
        auto *const height = new abstract_gui::spinner;
        height->isInitiallyEnabled = filter->parameter(filter_output_gate_c::PARAM_IS_HEIGHT_ENABLED);
        height->get_value = [=]{return filter->parameter(filter_output_gate_c::PARAM_HEIGHT);};
        height->set_value = [=](const int value){filter->set_parameter(filter_output_gate_c::PARAM_HEIGHT, value);};
        height->minValue = 0;
        height->maxValue = MAX_CAPTURE_HEIGHT;

        auto *const toggle = new abstract_gui::checkbox;
        toggle->get_value = [=]{return filter->parameter(filter_output_gate_c::PARAM_IS_HEIGHT_ENABLED);};
        toggle->set_value = [=](const bool value){
            filter->set_parameter(filter_output_gate_c::PARAM_IS_HEIGHT_ENABLED, value);
            height->set_enabled(value);
        };

        this->fields.push_back({"Y", {toggle, height}});
    }

    return;
}
