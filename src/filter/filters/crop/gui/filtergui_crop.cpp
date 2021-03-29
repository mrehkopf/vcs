/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/crop/filter_crop.h"
#include "filter/filters/crop/gui/filtergui_crop.h"

filtergui_crop_c::filtergui_crop_c(filter_c *const filter)
{
    {
        auto *const xPos = new filtergui_spinbox_s;

        xPos->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_X);};
        xPos->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_X, value);};
        xPos->maxValue = MAX_CAPTURE_WIDTH;
        xPos->minValue = 0;

        this->guiFields.push_back({"X", {xPos}});
    }

    {
        auto *const yPos = new filtergui_spinbox_s;

        yPos->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_Y);};
        yPos->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_Y, value);};
        yPos->maxValue = MAX_CAPTURE_HEIGHT;
        yPos->minValue = 0;

        this->guiFields.push_back({"Y", {yPos}});
    }

    {
        auto *const width = new filtergui_spinbox_s;

        width->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_WIDTH);};
        width->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_WIDTH, value);};
        width->maxValue = MAX_CAPTURE_WIDTH;
        width->minValue = 0;

        this->guiFields.push_back({"Width", {width}});
    }

    {
        auto *const height = new filtergui_spinbox_s;

        height->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_HEIGHT);};
        height->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_HEIGHT, value);};
        height->maxValue = MAX_CAPTURE_HEIGHT;
        height->minValue = 0;

        this->guiFields.push_back({"Height", {height}});
    }

    {
        auto *const blurType = new filtergui_combobox_s;

        blurType->get_value = [=]{return filter->parameter(filter_crop_c::PARAM_SCALER);};
        blurType->set_value = [=](const double value){filter->set_parameter(filter_crop_c::PARAM_SCALER, value);};
        blurType->items = {"Linear", "Nearest", "(Don't scale)"};

        this->guiFields.push_back({"Scaler", {blurType}});
    }
    
    return;
}
