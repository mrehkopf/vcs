/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h"
#include "common/globals.h"

filtergui_source_fps_estimate_c::filtergui_source_fps_estimate_c(abstract_filter_c *const filter)
{
    {
        auto *threshold = new abstract_gui::spinner;
        threshold->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_THRESHOLD);};
        threshold->set_value = [=](const int value){filter->set_parameter(filter_frame_rate_c::PARAM_THRESHOLD, value);};
        threshold->minValue = 0;
        threshold->maxValue = 255;

        this->fields.push_back({"Threshold", {threshold}});
    }

    {
        auto *singleRowNumber = new abstract_gui::spinner;
        singleRowNumber->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_SINGLE_ROW_NUMBER);};
        singleRowNumber->set_value = [=](const int value){filter->set_parameter(filter_frame_rate_c::PARAM_SINGLE_ROW_NUMBER, value);};
        singleRowNumber->minValue = 0;
        singleRowNumber->maxValue = MAX_CAPTURE_HEIGHT;
        singleRowNumber->isInitiallyEnabled = filter->parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);

        auto *const toggle = new abstract_gui::checkbox;
        toggle->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED);};
        toggle->set_value = [=](const bool value){
            filter->set_parameter(filter_frame_rate_c::PARAM_IS_SINGLE_ROW_ENABLED, value);
            singleRowNumber->set_enabled(value);
        };

        this->fields.push_back({"One row", {toggle, singleRowNumber}});
    }

    {
        auto *corner = new abstract_gui::combo_box;
        corner->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_CORNER);};
        corner->set_value = [=](const int value){filter->set_parameter(filter_frame_rate_c::PARAM_CORNER, value);};
        corner->items = {"Top left", "Top right", "Bottom right", "Bottom left"};

        this->fields.push_back({"Position", {corner}});
    }

    {
        auto *textColor = new abstract_gui::combo_box;
        textColor->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_TEXT_COLOR);};
        textColor->set_value = [=](const int value){filter->set_parameter(filter_frame_rate_c::PARAM_TEXT_COLOR, value);};
        textColor->items = {"Green", "Purple", "Red", "Yellow", "White"};

        this->fields.push_back({"Fg. color", {textColor}});
    }

    {
        auto *bgAlpha = new abstract_gui::spinner;
        bgAlpha->get_value = [=]{return filter->parameter(filter_frame_rate_c::PARAM_BG_ALPHA);};
        bgAlpha->set_value = [=](const int value){filter->set_parameter(filter_frame_rate_c::PARAM_BG_ALPHA, value);};
        bgAlpha->minValue = 0;
        bgAlpha->maxValue = 255;

        this->fields.push_back({"Bg. alpha", {bgAlpha}});
    }

    return;
}
