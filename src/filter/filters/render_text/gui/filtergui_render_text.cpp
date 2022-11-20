/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/gui/filtergui_render_text.h"

filtergui_render_text_c::filtergui_render_text_c(abstract_filter_c *const filter)
{
    {
        auto *const textEdit = new filtergui_textedit_s;
        textEdit->maxLength = filter_render_text_c::maxStringLength;
        textEdit->get_string = [=]{return filter->string_parameter(filter_render_text_c::PARAM_STRING);};
        textEdit->set_string = [=](const std::string &string){filter->set_parameter_string(filter_render_text_c::PARAM_STRING, string, filter_render_text_c::maxStringLength);};

        this->guiFields.push_back({"Text", {textEdit}});
    }

    {
        auto *const x = new filtergui_spinbox_s;
        x->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_X);};
        x->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_POS_X, value);};
        x->minValue = -MAX_CAPTURE_WIDTH;
        x->maxValue = MAX_CAPTURE_WIDTH;

        auto *const y = new filtergui_spinbox_s;
        y->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_Y);};
        y->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_POS_Y, value);};
        y->minValue = -MAX_CAPTURE_HEIGHT;
        y->maxValue = MAX_CAPTURE_HEIGHT;

        this->guiFields.push_back({"X, Y", {x, y}});
    }

    {
        auto *const scale = new filtergui_spinbox_s;
        scale->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_SCALE);};
        scale->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_SCALE, value);};
        scale->minValue = 1;
        scale->maxValue = 20;

        this->guiFields.push_back({"Scale", {scale}});
    }

    {
        auto *const font = new filtergui_combobox_s;
        font->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FONT);};
        font->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FONT, value);};
        font->items = {"5 \u00d7 3"};

        this->guiFields.push_back({"Font", {font}});
    }

    {
        auto *const textColor = new filtergui_combobox_s;
        textColor->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_COLOR);};
        textColor->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_COLOR, value);};
        textColor->items = {"Black", "Green", "Yellow", "White", "Transparent"};

        auto *const bgColor = new filtergui_combobox_s;
        bgColor->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR);};
        bgColor->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR, value);};
        bgColor->items = {"Black", "Green", "Yellow", "White", "Transparent"};

        auto *const label = new filtergui_label_s;
        label->text = "on";

        this->guiFields.push_back({"Color", {textColor, label, bgColor}});
    }

    return;
}
