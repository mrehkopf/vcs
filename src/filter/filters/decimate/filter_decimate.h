/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H
#define VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H

#include "filter/abstract_filter.h"

class filter_decimate_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_decimate_c)
    
    enum { PARAM_TYPE,
           PARAM_FACTOR };

    enum { SAMPLE_NEAREST = 0,
           SAMPLE_AVERAGE = 1 };

    filter_decimate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_FACTOR, 1},
            {PARAM_TYPE, SAMPLE_AVERAGE}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([this](abstract_gui_s *const gui)
        {
            auto *const factor = new abstract_gui::spinner;
            factor->get_value = [this]{return this->parameter(PARAM_FACTOR);};
            factor->set_value = [this](int value){this->set_parameter(PARAM_FACTOR, std::pow(2, value));};
            factor->minValue = 1;
            factor->maxValue = 4;
            gui->fields.push_back({"Factor", {factor}});

            auto *const sampler = new abstract_gui::combo_box;
            sampler->get_value = [this]{return this->parameter(PARAM_TYPE);};
            sampler->set_value = [this](int value){this->set_parameter(PARAM_TYPE, value);};
            sampler->items = {"Nearest", "Average"};
            gui->fields.push_back({"Sampler", {sampler}});
        });
    }

    void apply(image_s *const image) override
    {
        const int factor = this->parameter(PARAM_FACTOR);
        const unsigned type = this->parameter(PARAM_TYPE);
        const unsigned numColorChannels = (image->bitsPerPixel / 8);

        for (unsigned y = 0; y < image->resolution.h; y += factor)
        {
            for (unsigned x = 0; x < image->resolution.w; x += factor)
            {
                int ar = 0, ag = 0, ab = 0;

                if (type == SAMPLE_AVERAGE)
                {
                    for (int yd = 0; yd < factor; yd++)
                    {
                        for (int xd = 0; xd < factor; xd++)
                        {
                            const unsigned idx = ((x + xd) + (y + yd) * image->resolution.w) * numColorChannels;
                            ab += image->pixels[idx + 0];
                            ag += image->pixels[idx + 1];
                            ar += image->pixels[idx + 2];
                        }
                    }

                    ar /= (factor * factor);
                    ag /= (factor * factor);
                    ab /= (factor * factor);
                }
                else if (type == SAMPLE_NEAREST)
                {
                    const unsigned idx = (x + y * image->resolution.w) * numColorChannels;
                    ab = image->pixels[idx + 0];
                    ag = image->pixels[idx + 1];
                    ar = image->pixels[idx + 2];
                }

                for (int yd = 0; yd < factor; yd++)
                {
                    for (int xd = 0; xd < factor; xd++)
                    {
                        const unsigned idx = ((x + xd) + (y + yd) * image->resolution.w) * numColorChannels;
                        image->pixels[idx + 0] = ab;
                        image->pixels[idx + 1] = ag;
                        image->pixels[idx + 2] = ar;
                    }
                }
            }
        }

        return;
    }

    std::string uuid(void) const override { return "eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03"; }
    std::string name(void) const override { return "Decimate"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
