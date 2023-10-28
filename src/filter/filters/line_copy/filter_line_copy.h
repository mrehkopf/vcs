/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_LINE_COPY_FILTER_LINE_COPY_H
#define VCS_FILTER_FILTERS_LINE_COPY_FILTER_LINE_COPY_H

#include "filter/abstract_filter.h"
#include "common/globals.h"

// Copies a line (row or column) of pixels from one part of the image to another.
class filter_line_copy_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_line_copy_c)

    enum {
        PARAM_TO,
        PARAM_FROM,
        PARAM_AXIS,
        PARAM_WIDTH,
    };

    enum {
        HORIZONTAL,
        VERTICAL,
    };

    filter_line_copy_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_TO, 0},
            {PARAM_FROM, 1},
            {PARAM_WIDTH, 1},
            {PARAM_AXIS, HORIZONTAL},
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *from = filtergui::spinner(filter, PARAM_FROM);
            from->minValue = 0;
            from->maxValue = std::max(MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT);
            auto *to = filtergui::spinner(filter, PARAM_TO);
            to->minValue = 0;
            to->maxValue = std::max(MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT);
            gui->fields.push_back({"From/To", {from, to}});

            auto *width = filtergui::spinner(filter, PARAM_WIDTH);
            width->minValue = 0;
            width->maxValue = std::max(MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT);
            gui->fields.push_back({"Width", {width}});

            auto *axis = filtergui::combo_box(filter, PARAM_AXIS);
            axis->items = {"Horizontal", "Vertical"};
            gui->fields.push_back({"Axis", {axis}});
        });
    }

    void apply(image_s *const image) override
    {
        const unsigned width = this->parameter(PARAM_WIDTH);

        if (this->parameter(PARAM_AXIS) == HORIZONTAL)
        {
            const unsigned from = std::min(unsigned(this->parameter(PARAM_FROM)), (image->resolution.h - 1));
            const unsigned to = std::min(unsigned(this->parameter(PARAM_TO)), (image->resolution.h - 1));

            for (unsigned y = 0; y < width; y++)
            {
                const unsigned to_ = std::min((to + y), (image->resolution.h - 1));

                for (unsigned x = 0; x < image->resolution.w; x++)
                {
                    const unsigned srcIdx = (4 * (x + from * image->resolution.w));
                    const unsigned dstIdx = (4 * (x + to_ * image->resolution.w));
                    image->pixels[dstIdx+0] = image->pixels[srcIdx+0];
                    image->pixels[dstIdx+1] = image->pixels[srcIdx+1];
                    image->pixels[dstIdx+2] = image->pixels[srcIdx+2];
                }
            }
        }
        else
        {
            const unsigned from = std::min(unsigned(this->parameter(PARAM_FROM)), (image->resolution.w - 1));
            const unsigned to = std::min(unsigned(this->parameter(PARAM_TO)), (image->resolution.w - 1));

            for (unsigned x = 0; x < width; x++)
            {
                const unsigned to_ = std::min((to + x), (image->resolution.w - 1));

                for (unsigned y = 0; y < image->resolution.h; y++)
                {
                    const unsigned srcIdx = (4 * (from + y * image->resolution.w));
                    const unsigned dstIdx = (4 * (to_ + y * image->resolution.w));
                    image->pixels[dstIdx+0] = image->pixels[srcIdx+0];
                    image->pixels[dstIdx+1] = image->pixels[srcIdx+1];
                    image->pixels[dstIdx+2] = image->pixels[srcIdx+2];
                }
            }
        }

        return;
    }

    std::string uuid(void) const override { return "a51d1c1e-59dd-4548-8427-3e7d32e14c0d"; }
    std::string name(void) const override { return "Line copy"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

private:
};

#endif
