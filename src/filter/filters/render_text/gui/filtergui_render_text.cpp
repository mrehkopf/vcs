/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/gui/filtergui_render_text.h"
#include "common/globals.h"

filtergui_render_text_c::filtergui_render_text_c(abstract_filter_c *const filter)
{
    auto *textEdit = filtergui::text_edit(filter, filter_render_text_c::PARAM_STRING);
    textEdit->maxLength = filter_render_text_c::maxStringLength;
    this->fields.push_back({"Text", {textEdit}});

    auto *x = filtergui::spinner(filter, filter_render_text_c::PARAM_POS_X);
    x->minValue = -MAX_CAPTURE_WIDTH;
    x->maxValue = MAX_CAPTURE_WIDTH;
    auto *y = filtergui::spinner(filter, filter_render_text_c::PARAM_POS_Y);
    y->minValue = -MAX_CAPTURE_HEIGHT;
    y->maxValue = MAX_CAPTURE_HEIGHT;
    auto *align = filtergui::combo_box(filter, filter_render_text_c::PARAM_ALIGN);
    align->items = {"Left", "Center", "Right"};
    this->fields.push_back({"Position", {x, y, align}});

    auto *font = filtergui::combo_box(filter, filter_render_text_c::PARAM_FONT);
    font->items = {"Minimalist", "Retro Serif", "Retro Sans"};
    auto *scale = filtergui::spinner(filter, filter_render_text_c::PARAM_SCALE);
    scale->minValue = 1;
    scale->maxValue = 20;
    scale->suffix = "\u00d7";
    this->fields.push_back({"Font", {font, scale}});

    auto *fgRed = filtergui::spinner(filter, filter_render_text_c::PARAM_FG_COLOR_RED);
    fgRed->minValue = 0;
    fgRed->maxValue = 255;
    auto *fgGreen = filtergui::spinner(filter, filter_render_text_c::PARAM_FG_COLOR_GREEN);
    fgGreen->minValue = 0;
    fgGreen->maxValue = 255;
    auto *fgBlue = filtergui::spinner(filter, filter_render_text_c::PARAM_FG_COLOR_BLUE);
    fgBlue->minValue = 0;
    fgBlue->maxValue = 255;
    auto *fgAlpha = filtergui::spinner(filter, filter_render_text_c::PARAM_FG_COLOR_ALPHA);
    fgAlpha->minValue = 0;
    fgAlpha->maxValue = 255;
    this->fields.push_back({"Fg. color", {fgRed, fgGreen, fgBlue, fgAlpha}});

    auto *bgRed = filtergui::spinner(filter, filter_render_text_c::PARAM_BG_COLOR_RED);
    bgRed->minValue = 0;
    bgRed->maxValue = 255;
    auto *bgGreen = filtergui::spinner(filter, filter_render_text_c::PARAM_BG_COLOR_GREEN);
    bgGreen->minValue = 0;
    bgGreen->maxValue = 255;
    auto *bgBlue = filtergui::spinner(filter, filter_render_text_c::PARAM_BG_COLOR_BLUE);
    bgBlue->minValue = 0;
    bgBlue->maxValue = 255;
    auto *bgAlpha = filtergui::spinner(filter, filter_render_text_c::PARAM_BG_COLOR_ALPHA);
    bgAlpha->minValue = 0;
    bgAlpha->maxValue = 255;
    this->fields.push_back({"Bg. color", {bgRed, bgGreen, bgBlue, bgAlpha}});

    return;
}
