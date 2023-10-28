/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/input_gate/filter_input_gate.h"
#include "filter/filters/input_gate/gui/filtergui_input_gate.h"
#include "common/globals.h"
#include "common/refresh_rate.h"

filtergui_input_gate_c::filtergui_input_gate_c(abstract_filter_c *const filter)
{
    auto *width = filtergui::spinner(filter, filter_input_gate_c::PARAM_WIDTH);
    width->isInitiallyEnabled = filter->parameter(filter_input_gate_c::PARAM_IS_WIDTH_ENABLED);
    width->minValue = 0;
    width->maxValue = MAX_CAPTURE_WIDTH;
    auto *toggleWidth = filtergui::checkbox(filter, filter_input_gate_c::PARAM_IS_WIDTH_ENABLED);
    toggleWidth->on_change = [=](const bool value){
        filter->set_parameter(filter_input_gate_c::PARAM_IS_WIDTH_ENABLED, value);
        width->set_enabled(value);
    };
    this->fields.push_back({"X", {toggleWidth, width}});

    auto *height = filtergui::spinner(filter, filter_input_gate_c::PARAM_HEIGHT);
    height->isInitiallyEnabled = filter->parameter(filter_input_gate_c::PARAM_IS_HEIGHT_ENABLED);
    height->minValue = 0;
    height->maxValue = MAX_CAPTURE_HEIGHT;
    auto *toggleHeight = filtergui::checkbox(filter, filter_input_gate_c::PARAM_IS_HEIGHT_ENABLED);
    toggleHeight->on_change = [=](const bool value){
        filter->set_parameter(filter_input_gate_c::PARAM_IS_HEIGHT_ENABLED, value);
        height->set_enabled(value);
    };
    this->fields.push_back({"Y", {toggleHeight, height}});

    auto *hz = filtergui::double_spinner(filter, filter_input_gate_c::PARAM_HZ);
    hz->isInitiallyEnabled = filter->parameter(filter_input_gate_c::PARAM_IS_HZ_ENABLED);
    hz->minValue = 0;
    hz->maxValue = 1000;
    hz->numDecimals = refresh_rate_s::numDecimalsPrecision;
    auto *toggleHz = filtergui::checkbox(filter, filter_input_gate_c::PARAM_IS_HZ_ENABLED);
    toggleHz->on_change = [=](const bool value){
        filter->set_parameter(filter_input_gate_c::PARAM_IS_HZ_ENABLED, value);
        hz->set_enabled(value);
    };
    this->fields.push_back({"Hz", {toggleHz, hz}});

    return;
}
