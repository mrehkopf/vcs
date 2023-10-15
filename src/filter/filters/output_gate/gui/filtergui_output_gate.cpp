/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_gate/filter_output_gate.h"
#include "filter/filters/output_gate/gui/filtergui_output_gate.h"
#include "common/globals.h"

filtergui_output_gate_c::filtergui_output_gate_c(abstract_filter_c *const filter)
{
    auto *width = filtergui::spinner(filter, filter_output_gate_c::PARAM_WIDTH);
    width->isInitiallyEnabled = filter->parameter(filter_output_gate_c::PARAM_IS_WIDTH_ENABLED);
    width->minValue = 0;
    width->maxValue = MAX_CAPTURE_WIDTH;
    auto *toggleWidth = filtergui::checkbox(filter, filter_output_gate_c::PARAM_IS_WIDTH_ENABLED);
    toggleWidth->on_change = [=](const bool value){
        filter->set_parameter(filter_output_gate_c::PARAM_IS_WIDTH_ENABLED, value);
        width->set_enabled(value);
    };
    this->fields.push_back({"X", {toggleWidth, width}});

    auto *height = filtergui::spinner(filter, filter_output_gate_c::PARAM_HEIGHT);
    height->isInitiallyEnabled = filter->parameter(filter_output_gate_c::PARAM_IS_HEIGHT_ENABLED);
    height->minValue = 0;
    height->maxValue = MAX_CAPTURE_HEIGHT;
    auto *toggleHeight = filtergui::checkbox(filter, filter_output_gate_c::PARAM_IS_WIDTH_ENABLED);
    toggleHeight->on_change = [=](const bool value){
        filter->set_parameter(filter_output_gate_c::PARAM_IS_HEIGHT_ENABLED, value); height->set_enabled(value);
    };
    this->fields.push_back({"Y", {toggleHeight, height}});

    return;
}
