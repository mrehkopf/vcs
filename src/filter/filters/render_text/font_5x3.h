/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 */

#ifndef VCS_FILTER_FILTERS_RENDER_TEXT_FONT_5X3_H
#define VCS_FILTER_FILTERS_RENDER_TEXT_FONT_5X3_H

#include <unordered_map>
#include <vector>
#include "filter/filters/render_text/font.h"

// A retro-looking, uppercase-only, 5 x 3 font with monospaced digits.
class font_5x3_c : public font_c
{
public:
    unsigned cap_height() const override
    {
        return 5;
    }

    unsigned letter_spacing() const override
    {
        return 1;
    }

    unsigned line_spacing() const override
    {
        return (this->letter_spacing() * 2);
    }

    const font_glyph_s& glyph_at(const char key) const override
    {
        // This font supports only uppercase symbols.
        const char upperKey = std::toupper(key);

        return (
            (this->charset.find(upperKey) == this->charset.end())
            ? this->charset.at('?')
            : this->charset.at(upperKey)
        );
    }

private:
    #define X 1
    const std::unordered_map<char, font_glyph_s> charset = {
        {'?', {{{X,X,0},
                {0,0,X},
                {0,X,0},
                {0,0,0},
                {0,X,0}}}},

        {'!', {{{X},
                {X},
                {X},
                {0},
                {X}}}},

        {' ', {{{0}}}},

        {'.', {{{X}}}},

        {'\'', {{{0,X},
                 {X,0}}, 3}},

        {'\"', {{{0,X,0,X},
                 {X,0,X,0}}, 3}},

        {',', {{{0,X},
                {X,0}}, -1}},

        {':', {{{X},
                {0},
                {X}}, 1}},

        {'/', {{{0,0,X},
                {0,0,X},
                {0,X,0},
                {X,0,0},
                {X,0,0}}}},

        {'~', {{{0,X,0,X},
                {X,0,X,0}}, 1}},

        {'-', {{{X,X,X}}, 2}},

        {'+', {{{0,X,0},
                {X,X,X},
                {0,X,0}}, 1}},

        {'*', {{{X,0,X},
                {0,X,0},
                {X,0,X}}, 1}},

        {'[', {{{X,X},
                {X,0},
                {X,0},
                {X,0},
                {X,X}}}},

        {']', {{{X,X},
                {0,X},
                {0,X},
                {0,X},
                {X,X}}}},

        {'(', {{{0,X},
                {X,0},
                {X,0},
                {X,0},
                {0,X}}}},

        {')', {{{X,0},
                {0,X},
                {0,X},
                {0,X},
                {X,0}}}},

        {'A', {{{X,X,X},
                {X,0,X},
                {X,X,X},
                {X,0,X},
                {X,0,X}}}},

        {'B', {{{X,X,X},
                {X,0,X},
                {X,X,0},
                {X,0,X},
                {X,X,X}}}},

        {'C', {{{X,X,X},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,X,X}}}},

        {'D', {{{X,X,0},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {X,X,0}}}},

        {'E', {{{X,X,X},
                {X,0,0},
                {X,X,X},
                {X,0,0},
                {X,X,X}}}},

        {'F', {{{X,X,X},
                {X,0,0},
                {X,X,X},
                {X,0,0},
                {X,0,0}}}},

        {'G', {{{0,X,X,0},
                {X,0,0,0},
                {X,0,X,X},
                {X,0,0,X},
                {0,X,X,0}}}},

        {'H', {{{X,0,X},
                {X,0,X},
                {X,X,X},
                {X,0,X},
                {X,0,X}}}},

        {'I', {{{X,X,X},
                {0,X,0},
                {0,X,0},
                {0,X,0},
                {X,X,X}}}},

        {'J', {{{0,X,X},
                {0,0,X},
                {0,0,X},
                {0,0,X},
                {X,X,0}}}},

        {'K', {{{X,0,X},
                {X,0,X},
                {X,X,0},
                {X,0,X},
                {X,0,X}}}},

        {'L', {{{X,0,0},
                {X,0,0},
                {X,0,0},
                {X,0,0},
                {X,X,X}}}},

        {'M', {{{X,X,0,X,0},
                {X,0,X,0,X},
                {X,0,X,0,X},
                {X,0,X,0,X},
                {X,0,X,0,X}}}},

        {'N', {{{X,X,0},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {X,0,X}}}},

        {'O', {{{X,X,X},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {X,X,X}}}},

        {'P', {{{X,X,X},
                {X,0,X},
                {X,X,X},
                {X,0,0},
                {X,0,0}}}},

        {'Q', {{{0,X,X,0},
                {X,0,0,X},
                {X,0,0,X},
                {X,0,X,0},
                {0,X,0,X}}}},

        {'R', {{{X,X,X},
                {X,0,X},
                {X,X,0},
                {X,0,X},
                {X,0,X}}}},

        {'S', {{{X,X,X},
                {X,0,0},
                {X,X,X},
                {0,0,X},
                {X,X,X}}}},

        {'T', {{{X,X,X},
                {0,X,0},
                {0,X,0},
                {0,X,0},
                {0,X,0}}}},

        {'U', {{{X,0,X},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {X,X,X}}}},

        {'V', {{{X,0,X},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {0,X,0}}}},

        {'W', {{{X,0,X,0,X},
                {X,0,X,0,X},
                {X,0,X,0,X},
                {X,0,X,0,X},
                {X,X,0,X,0}}}},

        {'X', {{{X,0,X},
                {X,0,X},
                {0,X,0},
                {X,0,X},
                {X,0,X}}}},

        {'Y', {{{X,0,X},
                {X,0,X},
                {0,X,0},
                {0,X,0},
                {0,X,0}}}},

        {'Z', {{{X,X,X},
                {0,0,X},
                {0,X,0},
                {X,0,0},
                {X,X,X}}}},

        {'0', {{{X,X,X},
                {X,0,X},
                {X,0,X},
                {X,0,X},
                {X,X,X}}}},

        {'1', {{{0,X,0},
                {0,X,0},
                {0,X,0},
                {0,X,0},
                {0,X,0}}}},

        {'2', {{{X,X,X},
                {0,0,X},
                {X,X,X},
                {X,0,0},
                {X,X,X}}}},

        {'3', {{{X,X,X},
                {0,0,X},
                {X,X,X},
                {0,0,X},
                {X,X,X}}}},

        {'4', {{{X,0,X},
                {X,0,X},
                {X,X,X},
                {0,0,X},
                {0,0,X}}}},

        {'5', {{{X,X,X},
                {X,0,0},
                {X,X,X},
                {0,0,X},
                {X,X,X}}}},

        {'6', {{{X,X,X},
                {X,0,0},
                {X,X,X},
                {X,0,X},
                {X,X,X}}}},

        {'7', {{{X,X,X},
                {X,0,X},
                {X,0,X},
                {0,0,X},
                {0,0,X}}}},

        {'8', {{{X,X,X},
                {X,0,X},
                {X,X,X},
                {X,0,X},
                {X,X,X}}}},

        {'9', {{{X,X,X},
                {X,0,X},
                {X,X,X},
                {0,0,X},
                {X,X,X}}}},
    };
    #undef X
};

#endif
