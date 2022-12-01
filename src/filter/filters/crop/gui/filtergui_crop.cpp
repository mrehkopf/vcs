/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/crop/filter_crop.h"
#include "filter/filters/crop/gui/filtergui_crop.h"

filtergui_crop_c::filtergui_crop_c(abstract_filter_c *const filter)
{
    {
        auto *xPos = new filtergui_spinbox_s;
        xPos->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_X);};
        xPos->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_X, value);};
        xPos->maxValue = MAX_CAPTURE_WIDTH;
        xPos->minValue = 0;

        auto *yPos = new filtergui_spinbox_s;
        yPos->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_Y);};
        yPos->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_Y, value);};
        yPos->maxValue = MAX_CAPTURE_HEIGHT;
        yPos->minValue = 0;

        this->guiFields.push_back({"XY", {xPos, yPos}});
    }

    {
        auto *width = new filtergui_spinbox_s;
        width->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_WIDTH);};
        width->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_WIDTH, value);};
        width->maxValue = MAX_CAPTURE_WIDTH;
        width->minValue = 1;

        auto *height = new filtergui_spinbox_s;
        height->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_HEIGHT);};
        height->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_HEIGHT, value);};
        height->maxValue = MAX_CAPTURE_HEIGHT;
        height->minValue = 1;

        this->guiFields.push_back({"Size", {width, height}});
    }

    {
        auto *scaler = new filtergui_combobox_s;
        scaler->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_SCALER);};
        scaler->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_SCALER, value);};
        scaler->items = {"Linear", "Nearest", "None"};

        this->guiFields.push_back({"Scaler", {scaler}});
    }
    
    return;
}
