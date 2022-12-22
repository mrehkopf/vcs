/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/gui/filtergui_render_text.h"
#include "common/globals.h"

filtergui_render_text_c::filtergui_render_text_c(abstract_filter_c *const filter)
{
    {
        auto textEdit = new filtergui_textedit_s;
        textEdit->maxLength = filter_render_text_c::maxStringLength;
        textEdit->get_string = [=]{return filter->string_parameter(filter_render_text_c::PARAM_STRING);};
        textEdit->set_string = [=](const std::string &string){filter->set_parameter_string(filter_render_text_c::PARAM_STRING, string, filter_render_text_c::maxStringLength);};

        this->guiFields.push_back({"Text", {textEdit}});
    }

    {
        auto x = new filtergui_spinbox_s;
        x->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_X);};
        x->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_POS_X, value);};
        x->minValue = -MAX_CAPTURE_WIDTH;
        x->maxValue = MAX_CAPTURE_WIDTH;

        auto y = new filtergui_spinbox_s;
        y->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_Y);};
        y->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_POS_Y, value);};
        y->minValue = -MAX_CAPTURE_HEIGHT;
        y->maxValue = MAX_CAPTURE_HEIGHT;

        auto align = new filtergui_combobox_s;
        align->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_ALIGN);};
        align->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_ALIGN, value);};
        align->items = {"Left", "Center", "Right"};

        this->guiFields.push_back({"Position", {x, y, align}});
    }

    {
        auto font = new filtergui_combobox_s;
        font->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FONT);};
        font->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FONT, value);};
        font->items = {"Minimalist", "Retro Serif", "Retro Sans"};

        auto scale = new filtergui_spinbox_s;
        scale->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_SCALE);};
        scale->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_SCALE, value);};
        scale->minValue = 1;
        scale->maxValue = 20;
        scale->suffix = "\u00d7";

        this->guiFields.push_back({"Font", {font, scale}});
    }

    {
        auto fgRed = new filtergui_spinbox_s;
        fgRed->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_RED);};
        fgRed->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_RED, value);};
        fgRed->minValue = 0;
        fgRed->maxValue = 255;

        auto fgGreen = new filtergui_spinbox_s;
        fgGreen->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN);};
        fgGreen->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN, value);};
        fgGreen->minValue = 0;
        fgGreen->maxValue = 255;

        auto fgBlue = new filtergui_spinbox_s;
        fgBlue->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE);};
        fgBlue->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE, value);};
        fgBlue->minValue = 0;
        fgBlue->maxValue = 255;

        auto fgAlpha = new filtergui_spinbox_s;
        fgAlpha->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA);};
        fgAlpha->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA, value);};
        fgAlpha->minValue = 0;
        fgAlpha->maxValue = 255;

        this->guiFields.push_back({"Fg. color", {fgRed, fgGreen, fgBlue, fgAlpha}});
    }

    {
        auto bgRed = new filtergui_spinbox_s;
        bgRed->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_RED);};
        bgRed->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_RED, value);};
        bgRed->minValue = 0;
        bgRed->maxValue = 255;

        auto bgGreen = new filtergui_spinbox_s;
        bgGreen->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN);};
        bgGreen->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN, value);};
        bgGreen->minValue = 0;
        bgGreen->maxValue = 255;

        auto bgBlue = new filtergui_spinbox_s;
        bgBlue->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE);};
        bgBlue->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE, value);};
        bgBlue->minValue = 0;
        bgBlue->maxValue = 255;

        auto bgAlpha = new filtergui_spinbox_s;
        bgAlpha->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA);};
        bgAlpha->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA, value);};
        bgAlpha->minValue = 0;
        bgAlpha->maxValue = 255;

        this->guiFields.push_back({"Bg. color", {bgRed, bgGreen, bgBlue, bgAlpha}});
    }

    return;
}
