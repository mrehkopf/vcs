/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_RENDER_TEXT_FILTER_RENDER_TEXT_H
#define VCS_FILTER_FILTERS_RENDER_TEXT_FILTER_RENDER_TEXT_H

#include "filter/abstract_filter.h"
#include "filter/filters/render_text/gui/filtergui_render_text.h"

class font_c;

class filter_render_text_c : public abstract_filter_c
{
public:
    enum {
        PARAM_POS_X,
        PARAM_POS_Y,
        PARAM_SCALE,
        PARAM_FONT,
        PARAM_FG_COLOR_RED,
        PARAM_FG_COLOR_GREEN,
        PARAM_FG_COLOR_BLUE,
        PARAM_FG_COLOR_ALPHA,
        PARAM_BG_COLOR_RED,
        PARAM_BG_COLOR_GREEN,
        PARAM_BG_COLOR_BLUE,
        PARAM_BG_COLOR_ALPHA,
        PARAM_ALIGN,

        // REPURPOSE THESE FOR NEW PARAMETERS AS NEEDED.
        //
        // Since we use a kludge for storing the text string parameter - where the string
        // is actually stored character by character in as many unnamed parameter variables
        // at the end of the block of named parameters - we can't add new named parameters
        // without invalidating existing user data for the pre-existing parameters. So the
        // best can can do for now is to have a bunch of pre-allocated slots into which
        // future new parameters can be inserted as needed.
        PARAM_UNUSED1,
        PARAM_UNUSED2,
        PARAM_UNUSED3,
        PARAM_UNUSED4,
        PARAM_UNUSED5,
        PARAM_UNUSED6,
        PARAM_UNUSED7,
        PARAM_UNUSED8,
        PARAM_UNUSED9,
        PARAM_UNUSED10,

        // THIS MUST BE THE LAST ENUMERATOR IN THE LIST. This parameter holds the first
        // character of the user-given text string to be rendered. We'll programmatically
        // allocate a bunch of unnamed parameters following this one, where each parameter
        // holds one of the string's characters.
        PARAM_STRING
    };

    enum {
        FONT_MINIMALIST,
        FONT_RETRO_SERIF,
        FONT_RETRO_SANS_SERIF,
    };

    enum {
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT,
    };

    static const std::size_t maxStringLength = 255;

    filter_render_text_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_POS_X, 0},
            {PARAM_POS_Y, 0},
            {PARAM_FG_COLOR_RED, 255},
            {PARAM_FG_COLOR_GREEN, 255},
            {PARAM_FG_COLOR_BLUE, 0},
            {PARAM_FG_COLOR_ALPHA, 255},
            {PARAM_BG_COLOR_RED, 0},
            {PARAM_BG_COLOR_GREEN, 0},
            {PARAM_BG_COLOR_BLUE, 0},
            {PARAM_BG_COLOR_ALPHA, 255},
            {PARAM_SCALE, 1},
            {PARAM_FONT, FONT_MINIMALIST},
            {PARAM_ALIGN, ALIGN_LEFT},
            {PARAM_UNUSED1, 0},
            {PARAM_UNUSED2, 0},
            {PARAM_UNUSED3, 0},
            {PARAM_UNUSED4, 0},
            {PARAM_UNUSED5, 0},
            {PARAM_UNUSED6, 0},
            {PARAM_UNUSED7, 0},
            {PARAM_UNUSED8, 0},
            {PARAM_UNUSED9, 0},
            {PARAM_UNUSED10, 0},
            {PARAM_STRING, 0},
        }, initialParamValues)
    {
        // Manually allocate more space in the parameter array for storing the user's string.
        auto params = this->parameters();
        params.resize(PARAM_STRING + filter_render_text_c::maxStringLength + 1); // +1 for null terminator.
        this->set_parameters(params);

        this->gui = new filtergui_render_text_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_render_text_c)

    std::string uuid(void) const override { return "b87db194-d76a-4a15-9f0f-a5cd9f2a209b"; }
    std::string name(void) const override { return "Render text"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }
    void apply(image_s *const image) override;

private:
};

#endif
