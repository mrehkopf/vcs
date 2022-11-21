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
        auto *const width = new filtergui_spinbox_s;
        width->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_WIDTH);};
        width->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_WIDTH, value);};
        width->minValue = MIN_OUTPUT_WIDTH;
        width->maxValue = MAX_OUTPUT_WIDTH;

        auto *const separator = new filtergui_label_s;
        separator->text = "\u00d7";

        auto *const height = new filtergui_spinbox_s;
        height->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_HEIGHT);};
        height->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_HEIGHT, value);};
        height->minValue = MIN_OUTPUT_WIDTH;
        height->maxValue = MAX_OUTPUT_WIDTH;

        this->guiFields.push_back({"Resolution", {width, separator, height}});
    }

    {
        auto *const scalerName = new filtergui_combobox_s;
        scalerName->get_value = [=]{return filter->parameter(filter_output_scaler_c::PARAM_SCALER_NAME);};
        scalerName->set_value = [=](const double value){filter->set_parameter(filter_output_scaler_c::PARAM_SCALER_NAME, value);};
        scalerName->items = ks_scaler_names();

        this->guiFields.push_back({"Sampling", {scalerName}});
    }

    return;
}
