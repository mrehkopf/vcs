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
    const int textColorId = this->parameter(filter_render_text_c::PARAM_COLOR);
    const int bgColorId = this->parameter(filter_render_text_c::PARAM_BG_COLOR);
    const int fontId = this->parameter(filter_render_text_c::PARAM_FONT);
    const int scale = this->parameter(filter_render_text_c::PARAM_SCALE);
    const unsigned x = this->parameter(filter_render_text_c::PARAM_POS_X);
    const unsigned y = this->parameter(filter_render_text_c::PARAM_POS_Y);

    const auto textColor = ([textColorId]()->std::vector<uint8_t>
    {
        switch (textColorId)
        {
            case COLOR_BLACK: return {0, 0, 0};
            case COLOR_GREEN: return {0, 255, 0};
            case COLOR_WHITE: return {255, 255, 255};
            case COLOR_YELLOW: return {0, 255, 255};
            case COLOR_TRANSPARENT: return {};
            default: return {255, 0, 255};
        }
    })();

    const auto bgColor = ([bgColorId]()->std::vector<uint8_t>
    {
        switch (bgColorId)
        {
            case COLOR_BLACK: return {0, 0, 0};
            case COLOR_GREEN: return {0, 255, 0};
            case COLOR_WHITE: return {255, 255, 255};
            case COLOR_YELLOW: return {0, 255, 255};
            case COLOR_TRANSPARENT: return {};
            default: return {255, 0, 255};
        }
    })();

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

    font->render(text, {pixels, r}, x, y, scale, textColor, bgColor);

    return;
}
