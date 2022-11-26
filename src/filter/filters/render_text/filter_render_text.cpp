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

void filter_render_text_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    const std::string text = this->string_parameter(filter_render_text_c::PARAM_STRING);
    const uint8_t fgColorRed = this->parameter(filter_render_text_c::PARAM_FG_COLOR_RED);
    const uint8_t fgColorGreen = this->parameter(filter_render_text_c::PARAM_FG_COLOR_GREEN);
    const uint8_t fgColorBlue = this->parameter(filter_render_text_c::PARAM_FG_COLOR_BLUE);
    const uint8_t fgColorAlpha = this->parameter(filter_render_text_c::PARAM_FG_COLOR_ALPHA);
    const uint8_t bgColorRed = this->parameter(filter_render_text_c::PARAM_BG_COLOR_RED);
    const uint8_t bgColorGreen = this->parameter(filter_render_text_c::PARAM_BG_COLOR_GREEN);
    const uint8_t bgColorBlue = this->parameter(filter_render_text_c::PARAM_BG_COLOR_BLUE);
    const uint8_t bgColorAlpha = this->parameter(filter_render_text_c::PARAM_BG_COLOR_ALPHA);
    const int fontId = this->parameter(filter_render_text_c::PARAM_FONT);
    const int scale = this->parameter(filter_render_text_c::PARAM_SCALE);
    const unsigned x = this->parameter(filter_render_text_c::PARAM_POS_X);
    const unsigned y = this->parameter(filter_render_text_c::PARAM_POS_Y);

    const font_c *const font = ([fontId]()->font_c*
    {
        switch (fontId)
        {
            case FONT_MINIMALIST: return new font_5x3_c;
            case FONT_RETRO_SERIF: return new font_10x6_serif_c;
            case FONT_RETRO_SANS_SERIF: return new font_10x6_sans_serif_c;
            default: return new font_5x3_c;
        }
    })();

    font->render(
        text,
        {pixels, r}, x, y, scale,
        {fgColorBlue, fgColorGreen, fgColorRed, fgColorAlpha},
        {bgColorBlue, bgColorGreen, bgColorRed, bgColorAlpha}
    );

    return;
}
