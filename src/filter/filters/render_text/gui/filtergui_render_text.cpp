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
        auto textEdit = new abstract_gui::text_edit;
        textEdit->maxLength = filter_render_text_c::maxStringLength;
        textEdit->get_text = [=]{return filter->string_parameter(filter_render_text_c::PARAM_STRING);};
        textEdit->set_text = [=](const std::string &text){filter->set_parameter_string(filter_render_text_c::PARAM_STRING, text, filter_render_text_c::maxStringLength);};

        this->fields.push_back({"Text", {textEdit}});
    }

    {
        auto x = new abstract_gui::spinner;
        x->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_X);};
        x->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_POS_X, value);};
        x->minValue = -MAX_CAPTURE_WIDTH;
        x->maxValue = MAX_CAPTURE_WIDTH;

        auto y = new abstract_gui::spinner;
        y->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_POS_Y);};
        y->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_POS_Y, value);};
        y->minValue = -MAX_CAPTURE_HEIGHT;
        y->maxValue = MAX_CAPTURE_HEIGHT;

        auto align = new abstract_gui::combo_box;
        align->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_ALIGN);};
        align->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_ALIGN, value);};
        align->items = {"Left", "Center", "Right"};

        this->fields.push_back({"Position", {x, y, align}});
    }

    {
        auto font = new abstract_gui::combo_box;
        font->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FONT);};
        font->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_FONT, value);};
        font->items = {"Minimalist", "Retro Serif", "Retro Sans"};

        auto scale = new abstract_gui::spinner;
        scale->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_SCALE);};
        scale->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_SCALE, value);};
        scale->minValue = 1;
        scale->maxValue = 20;
        scale->suffix = "\u00d7";

        this->fields.push_back({"Font", {font, scale}});
    }

    {
        auto fgRed = new abstract_gui::spinner;
        fgRed->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_RED);};
        fgRed->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_RED, value);};
        fgRed->minValue = 0;
        fgRed->maxValue = 255;

        auto fgGreen = new abstract_gui::spinner;
        fgGreen->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN);};
        fgGreen->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN, value);};
        fgGreen->minValue = 0;
        fgGreen->maxValue = 255;

        auto fgBlue = new abstract_gui::spinner;
        fgBlue->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE);};
        fgBlue->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE, value);};
        fgBlue->minValue = 0;
        fgBlue->maxValue = 255;

        auto fgAlpha = new abstract_gui::spinner;
        fgAlpha->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA);};
        fgAlpha->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA, value);};
        fgAlpha->minValue = 0;
        fgAlpha->maxValue = 255;

        this->fields.push_back({"Fg. color", {fgRed, fgGreen, fgBlue, fgAlpha}});
    }

    {
        auto bgRed = new abstract_gui::spinner;
        bgRed->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_RED);};
        bgRed->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_RED, value);};
        bgRed->minValue = 0;
        bgRed->maxValue = 255;

        auto bgGreen = new abstract_gui::spinner;
        bgGreen->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN);};
        bgGreen->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN, value);};
        bgGreen->minValue = 0;
        bgGreen->maxValue = 255;

        auto bgBlue = new abstract_gui::spinner;
        bgBlue->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE);};
        bgBlue->set_value = [=](const int value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE, value);};
        bgBlue->minValue = 0;
        bgBlue->maxValue = 255;

        auto bgAlpha = new abstract_gui::spinner;
        bgAlpha->get_value = [=]{return filter->parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA);};
        bgAlpha->set_value = [=](const double value){filter->set_parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA, value);};
        bgAlpha->minValue = 0;
        bgAlpha->maxValue = 255;

        this->fields.push_back({"Bg. color", {bgRed, bgGreen, bgBlue, bgAlpha}});
    }

    return;
}
