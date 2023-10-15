/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h"
#include "common/globals.h"

filtergui_source_fps_estimate_c::filtergui_source_fps_estimate_c(abstract_filter_c *const filter)
{
    auto *threshold = filtergui::spinner(filter, filter_frame_rate_c::PARAM_THRESHOLD);
    threshold->minValue = 0;
    threshold->maxValue = 255;
    this->fields.push_back({"Threshold", {threshold}});

    auto *singleRowNumber = filtergui::spinner(filter, filter_frame_rate_c::PARAM_SINGLE_ROW_NUMBER);
    singleRowNumber->isInitiallyEnabled = filter->parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
    singleRowNumber->minValue = 0;
    singleRowNumber->maxValue = MAX_CAPTURE_HEIGHT;
    auto *toggleRow = filtergui::checkbox(filter, filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);
    toggleRow->on_change = [=](const bool value){
        filter->set_parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED, value);
        singleRowNumber->set_enabled(value);
    };
    this->fields.push_back({"One row", {toggleRow, singleRowNumber}});

    auto *corner = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_CORNER);
    corner->items = {"Top left", "Top right", "Bottom right", "Bottom left"};
    this->fields.push_back({"Position", {corner}});

    auto *textColor = filtergui::combo_box(filter, filter_frame_rate_c::PARAM_TEXT_COLOR);
    textColor->items = {"Green", "Purple", "Red", "Yellow", "White"};
    this->fields.push_back({"Fg. color", {textColor}});

    auto *bgAlpha = filtergui::spinner(filter, filter_frame_rate_c::PARAM_BG_ALPHA);
    bgAlpha->minValue = 0;
    bgAlpha->maxValue = 255;
    this->fields.push_back({"Bg. alpha", {bgAlpha}});

    return;
}
