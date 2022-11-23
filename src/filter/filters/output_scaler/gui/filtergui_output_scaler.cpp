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
        auto *const scaledWidth = new filtergui_spinbox_s;
        scaledWidth->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_WIDTH);};
        scaledWidth->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_WIDTH, value);};
        scaledWidth->minValue = MIN_OUTPUT_WIDTH;
        scaledWidth->maxValue = MAX_OUTPUT_WIDTH;

        auto *const separator = new filtergui_label_s;
        separator->text = "\u00d7";

        auto *const scaledHeight = new filtergui_spinbox_s;
        scaledHeight->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_HEIGHT);};
        scaledHeight->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_HEIGHT, value);};
        scaledHeight->minValue = MIN_OUTPUT_HEIGHT;
        scaledHeight->maxValue = MAX_OUTPUT_HEIGHT;

        this->guiFields.push_back({"Scale to", {scaledWidth, separator, scaledHeight}});
    }

    {
        auto *const scalerName = new filtergui_combobox_s;
        scalerName->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_SCALER);};
        scalerName->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_SCALER, value);};
        scalerName->items = ks_scaler_names();

        this->guiFields.push_back({"Sampler", {scalerName}});
    }

    {
        auto *const topPadding = new filtergui_spinbox_s;
        topPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_TOP);};
        topPadding->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_TOP, value);};
        topPadding->minValue = 0;
        topPadding->maxValue = MAX_OUTPUT_HEIGHT;
        topPadding->alignment = filtergui_alignment_e::center;

        auto *const bottomPadding = new filtergui_spinbox_s;
        bottomPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM);};
        bottomPadding->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM, value);};
        bottomPadding->minValue = 0;
        bottomPadding->maxValue = MAX_OUTPUT_HEIGHT;
        bottomPadding->alignment = filtergui_alignment_e::center;

        auto *const leftPadding = new filtergui_spinbox_s;
        leftPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_LEFT);};
        leftPadding->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_LEFT, value);};
        leftPadding->minValue = 0;
        leftPadding->maxValue = MAX_OUTPUT_WIDTH;
        leftPadding->alignment = filtergui_alignment_e::center;

        auto *const rightPadding = new filtergui_spinbox_s;
        rightPadding->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT);};
        rightPadding->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT, value);};
        rightPadding->minValue = 0;
        rightPadding->maxValue = MAX_OUTPUT_WIDTH;
        rightPadding->alignment = filtergui_alignment_e::center;

        this->guiFields.push_back({"Padding", {topPadding}});
        this->guiFields.push_back({"", {leftPadding, rightPadding}});
        this->guiFields.push_back({"", {bottomPadding}});
    }

    return;
}
