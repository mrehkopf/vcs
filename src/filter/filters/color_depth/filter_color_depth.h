/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_COLOR_DEPTH_FILTER_COLOR_DEPTH_H
#define VCS_FILTER_FILTERS_COLOR_DEPTH_FILTER_COLOR_DEPTH_H

#include "filter/abstract_filter.h"

class filter_color_depth_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_color_depth_c)

    enum { PARAM_BIT_COUNT_RED,
           PARAM_BIT_COUNT_GREEN,
           PARAM_BIT_COUNT_BLUE };
    
    filter_color_depth_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_BIT_COUNT_RED, 8},
            {PARAM_BIT_COUNT_GREEN, 8},
            {PARAM_BIT_COUNT_BLUE, 8}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *red = filtergui::spinner(filter, PARAM_BIT_COUNT_RED);
            red->minValue = 1;
            red->maxValue = 8;

            auto *green = filtergui::spinner(filter, PARAM_BIT_COUNT_GREEN);
            green->minValue = 1;
            green->maxValue = 8;

            auto *blue = filtergui::spinner(filter, PARAM_BIT_COUNT_BLUE);
            blue->minValue = 1;
            blue->maxValue = 8;

            gui->fields.push_back({"", {red, green, blue}});
        });
    }

    void apply(image_s *const image) override
    {
        const unsigned maskRed   = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_RED)));
        const unsigned maskGreen = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_GREEN)));
        const unsigned maskBlue  = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_BLUE)));
        const unsigned numImageBytes = (image->resolution.w * image->resolution.h * (image->bitsPerPixel / 8));

        // Assumes 32-bit pixels (BGRA8888).
        for (unsigned i = 0; i < numImageBytes; i += 4)
        {
            image->pixels[i + 0] &= maskBlue;
            image->pixels[i + 1] &= maskGreen;
            image->pixels[i + 2] &= maskRed;
        }

        return;
    }

    std::string name(void) const override { return "Color depth"; }
    std::string uuid(void) const override { return "c87f6967-b82d-4a10-b74f-9923f5ed00f8"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
