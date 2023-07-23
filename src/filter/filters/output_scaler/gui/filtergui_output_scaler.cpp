/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "filter/filters/output_scaler/gui/filtergui_output_scaler.h"
#include "scaler/scaler.h"

filtergui_output_scaler_c::filtergui_output_scaler_c(abstract_filter_c *const filter)
{
    {
        auto *const scaledWidth = new abstract_gui::spinner;
        scaledWidth->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_WIDTH);};
        scaledWidth->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_WIDTH, value);};
        scaledWidth->minValue = MIN_OUTPUT_WIDTH;
        scaledWidth->maxValue = MAX_OUTPUT_WIDTH;

        auto *const separator = new abstract_gui::label;
        separator->text = "\u00d7";

        auto *const scaledHeight = new abstract_gui::spinner;
        scaledHeight->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_HEIGHT);};
        scaledHeight->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_HEIGHT, value);};
        scaledHeight->minValue = MIN_OUTPUT_HEIGHT;
        scaledHeight->maxValue = MAX_OUTPUT_HEIGHT;

        this->fields.push_back({"Scale to", {scaledWidth, separator, scaledHeight}});
    }

    {
        auto *const scalerName = new abstract_gui::combo_box;
        scalerName->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_SCALER);};
        scalerName->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_SCALER, value);};
        scalerName->items = ks_scaler_names();

        this->fields.push_back({"Sampler", {scalerName}});
    }

    {
        auto *const topPadding = new abstract_gui::spinner;
        topPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_TOP);};
        topPadding->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_TOP, value);};
        topPadding->minValue = 0;
        topPadding->maxValue = MAX_OUTPUT_HEIGHT;
        topPadding->alignment = abstract_gui::horizontal_align::center;

        auto *const bottomPadding = new abstract_gui::spinner;
        bottomPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM);};
        bottomPadding->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM, value);};
        bottomPadding->minValue = 0;
        bottomPadding->maxValue = MAX_OUTPUT_HEIGHT;
        bottomPadding->alignment = abstract_gui::horizontal_align::center;

        auto *const leftPadding = new abstract_gui::spinner;
        leftPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_LEFT);};
        leftPadding->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_LEFT, value);};
        leftPadding->minValue = 0;
        leftPadding->maxValue = MAX_OUTPUT_WIDTH;
        leftPadding->alignment = abstract_gui::horizontal_align::center;

        auto *const rightPadding = new abstract_gui::spinner;
        rightPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT);};
        rightPadding->set_value = [=](const int value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT, value);};
        rightPadding->minValue = 0;
        rightPadding->maxValue = MAX_OUTPUT_WIDTH;
        rightPadding->alignment = abstract_gui::horizontal_align::center;

        this->fields.push_back({"Padding", {topPadding}});
        this->fields.push_back({"", {leftPadding, rightPadding}});
        this->fields.push_back({"", {bottomPadding}});
    }

    return;
}
