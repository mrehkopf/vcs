/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 */

#ifndef VCS_FILTER_FILTERS_RENDER_TEXT_FONT_H
#define VCS_FILTER_FILTERS_RENDER_TEXT_FONT_H

#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <cmath>
#include "filter/filters/render_text/filter_render_text.h"
#include "display/display.h"
#include "common/globals.h"
#include "common/assert.h"

struct font_glyph_s
{
    std::vector<std::vector<bool>> rows;
    int baseline;

    unsigned width(void) const
    {
        return (this->rows.empty()? 0 : this->rows.at(0).size());
    }

    unsigned height(void) const
    {
        return this->rows.size();
    }

    void render(
        image_s *const dstImage,
        const int x,
        const int y,
        const unsigned scale = 1,
        const std::vector<uint8_t> color = {0, 0, 0}
    ) const
    {
        k_assert_optional((color.size() <= (dstImage->bitsPerPixel / 8)), "Malformed color for glyph rendering.");

        for (unsigned glyphY = 0; glyphY < this->height(); glyphY++)
        {
            for (unsigned glyphX = 0; glyphX < this->width(); glyphX++)
            {
                if (!this->rows.at(this->height() - glyphY - 1).at(glyphX))
                {
                    continue;
                }

                const int pixelX = (x + int(glyphX * scale));
                const int pixelY = (y - int(glyphY * scale) - (this->baseline * scale));

                for (unsigned repeatY = 0; repeatY < scale; repeatY++)
                {
                    const int scaledPixelY = (pixelY + repeatY);

                    if ((scaledPixelY < 0) || (unsigned(scaledPixelY) >= dstImage->resolution.h))
                    {
                        continue;
                    }

                    for (unsigned repeatX = 0; repeatX < scale; repeatX++)
                    {
                        const int scaledPixelX = (pixelX + repeatX);

                        if ((scaledPixelX < 0) || (unsigned(scaledPixelX) >= dstImage->resolution.w))
                        {
                            continue;
                        }

                        const unsigned idx = ((scaledPixelX + scaledPixelY * dstImage->resolution.w) * (dstImage->bitsPerPixel / 8));
                        k_assert_optional(
                            (idx < (color.size() + (dstImage->resolution.w * dstImage->resolution.h * (dstImage->bitsPerPixel / 8)))),
                            "Image buffer overflow in string rendering."
                        );

                        // Alpha blend.
                        if (color.size() == 4)
                        {
                            dstImage->pixels[idx + 0] = std::lerp(dstImage->pixels[idx + 0], color[0], (color[3] / 255.0));
                            dstImage->pixels[idx + 1] = std::lerp(dstImage->pixels[idx + 1], color[1], (color[3] / 255.0));
                            dstImage->pixels[idx + 2] = std::lerp(dstImage->pixels[idx + 2], color[2], (color[3] / 255.0));
                        }
                        else
                        {
                            for (unsigned i = 0; i < color.size(); i++)
                            {
                                dstImage->pixels[idx + i] = color[i];
                            }
                        }
                    }
                }
            }
        }

        return;
    }
};

class font_c
{
public:
    // Returns the width in pixels of the given string using this font. If the string
    // contains newlines (must be Unix-style), returns the width of the longest line.
    // If the string contains characters that are not supported by this font, the width
    // is calculated based on those characters replaced with the font's fallback glyph
    // (e.g. '?').
    unsigned width_of(const std::string &string, const bool includeBackground = true) const
    {
        if (string.empty())
        {
            return includeBackground;
        }

        std::vector<unsigned> lineWidths = {0};

        for (const char ch: string)
        {
            if (ch == '\n')
            {
                lineWidths.push_back(0);
                continue;
            }

            const font_glyph_s &glyph = this->glyph_at(ch);
            lineWidths.back() += (glyph.width() + this->letter_spacing());
        }

        return (
            *std::max_element(lineWidths.begin(), lineWidths.end()) -
            this->letter_spacing() +
            (includeBackground? 2 : 0)
        );
    }

    // Returns the height in pixels of the given string using this font. If the string
    // contains newlines (must be Unix-style), returns the combined height of the lines.
    // If the string contains characters that are not supported by this font, the height
    // is calculated based on those characters replaced with the font's fallback glyph
    // (e.g. '?').
    unsigned height_of(const std::string &string, const bool includeBackground = true) const
    {
        if (string.empty())
        {
            return includeBackground;
        }

        std::vector<unsigned> lineHeights = {this->cap_height()};

        for (const char ch: string)
        {
            if (ch == '\n')
            {
                lineHeights.push_back(this->cap_height());
                continue;
            }

            const font_glyph_s &glyph = this->glyph_at(ch);
            lineHeights.back() = std::max(lineHeights.back(), (this->cap_height() - glyph.baseline));
        }

        return (
            std::accumulate(lineHeights.begin(), lineHeights.end(), 0) +
            ((lineHeights.size() - 1) * this->line_spacing()) +
            (includeBackground? 2 : 0)
        );
    }

    // Renders the given string into the given image using this font.
    void render(
        const std::string &string,
        image_s *const dstImage,
        int x,
        int y,
        const unsigned scale = 1,
        const std::vector<uint8_t> color = {0, 0, 0},
        const std::vector<uint8_t> bgColor = {},
        const unsigned alignment = filter_render_text_c::ALIGN_LEFT
    ) const
    {
        k_assert_optional((color.size() <= (dstImage->bitsPerPixel / 8)), "Malformed color for string rendering.");
        k_assert_optional((bgColor.size() <= (dstImage->bitsPerPixel / 8)), "Malformed background color for string rendering.");

        if (string.empty())
        {
            return;
        }

        const unsigned stringWidth = (this->width_of(string) * scale);
        const unsigned stringHeight = (this->height_of(string) * scale);

        x += scale;
        y += (this->cap_height() * scale);

        switch (alignment)
        {
            default:
            case filter_render_text_c::ALIGN_LEFT: break;
            case filter_render_text_c::ALIGN_RIGHT: x -= stringWidth; break;
            case filter_render_text_c::ALIGN_CENTER: x -= (stringWidth / 2); break;
        }

        // Render the background.
        if (!bgColor.empty())
        {
            for (unsigned bgY = 0; bgY < stringHeight; bgY++)
            {
                for (unsigned bgX = 0; bgX < stringWidth; bgX++)
                {
                    const unsigned x_ = (bgX + (x - scale));
                    const unsigned y_ = (bgY + (y - (this->cap_height() * scale)));
                    if ((x_ >= dstImage->resolution.w || (y_ >= dstImage->resolution.h)))
                    {
                        continue;
                    }

                    const unsigned idx = ((x_ + y_ * dstImage->resolution.w) * (dstImage->bitsPerPixel / 8));
                    k_assert_optional(
                        (idx < (bgColor.size() + (dstImage->resolution.w * dstImage->resolution.h * (dstImage->bitsPerPixel / 8)))),
                        "Image buffer overflow in string rendering."
                    );

                    // Alpha blend.
                    if (bgColor.size() == 4)
                    {
                        dstImage->pixels[idx + 0] = std::lerp(dstImage->pixels[idx + 0], bgColor[0], (bgColor[3] / 255.0));
                        dstImage->pixels[idx + 1] = std::lerp(dstImage->pixels[idx + 1], bgColor[1], (bgColor[3] / 255.0));
                        dstImage->pixels[idx + 2] = std::lerp(dstImage->pixels[idx + 2], bgColor[2], (bgColor[3] / 255.0));
                    }
                    else
                    {
                        for (unsigned i = 0; i < bgColor.size(); i++)
                        {
                            dstImage->pixels[idx + i] = bgColor[i];
                        }
                    }
                }
            }
        }

        // Render the string.
        if (!color.empty())
        {
            int runningX = x;
            int runningY = y;
            int curLineHeight = this->cap_height();

            for (const char ch: string)
            {
                if (ch == '\n')
                {
                    runningY += ((curLineHeight + this->line_spacing()) * scale);
                    runningX = x;
                    curLineHeight = this->cap_height();
                    continue;
                }

                const font_glyph_s &glyph = this->glyph_at(ch);

                glyph.render(dstImage, runningX, runningY, scale, color);
                runningX += ((glyph.width() * scale) + (this->letter_spacing() * scale));
                curLineHeight = std::max(curLineHeight, (int(this->cap_height()) - glyph.baseline));
            }
        }

        return;
    }

    virtual unsigned cap_height() const = 0;
    virtual unsigned letter_spacing() const = 0;
    virtual unsigned line_spacing() const = 0;
    virtual const font_glyph_s& glyph_at(const char key) const = 0;
};

#endif
