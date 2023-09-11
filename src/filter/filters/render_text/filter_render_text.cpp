/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <algorithm>
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/font_5x3.h"
#include "filter/filters/render_text/font_10x6_serif.h"
#include "filter/filters/render_text/font_10x6_sans_serif.h"
#include "display/display.h"

static const std::unordered_map<int, font_c*> FONTS = {
    {filter_render_text_c::FONT_MINIMALIST, new font_5x3_c},
    {filter_render_text_c::FONT_RETRO_SERIF, new font_10x6_serif_c},
    {filter_render_text_c::FONT_RETRO_SANS_SERIF, new font_10x6_sans_serif_c},
};

void filter_render_text_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const std::string text = this->string_parameter(filter_render_text_c::PARAM_STRING);
    const uint8_t fgColorRed = this->parameter(filter_render_text_c::PARAM_FG_COLOR_RED);
    const uint8_t fgColorGreen = this->parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN);
    const uint8_t fgColorBlue = this->parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE);
    const uint8_t fgColorAlpha = this->parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA);
    const uint8_t bgColorRed = this->parameter(filter_render_text_c::PARAM_BG_COLOR_RED);
    const uint8_t bgColorGreen = this->parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN);
    const uint8_t bgColorBlue = this->parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE);
    const uint8_t bgColorAlpha = this->parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA);
    const unsigned fontId = this->parameter(filter_render_text_c::PARAM_FONT);
    const unsigned scale = this->parameter(filter_render_text_c::PARAM_SCALE);
    const unsigned alignment = this->parameter(filter_render_text_c::PARAM_ALIGN);
    const unsigned x = this->parameter(filter_render_text_c::PARAM_POS_X);
    const unsigned y = this->parameter(filter_render_text_c::PARAM_POS_Y);

    FONTS.at(fontId)->render(
        text,
        image,
        x, y, scale,
        {fgColorBlue, fgColorGreen, fgColorRed, fgColorAlpha},
        {bgColorBlue, bgColorGreen, bgColorRed, bgColorAlpha},
        alignment
    );

    return;
}
