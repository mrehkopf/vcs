/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H
#define VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H

#include "filter/abstract_filter.h"
#include "filter/filters/decimate/gui/filtergui_decimate.h"

class filter_decimate_c : public abstract_filter_c
{
public:
    enum { PARAM_TYPE,
           PARAM_FACTOR };

    enum { SAMPLE_NEAREST = 0,
           SAMPLE_AVERAGE = 1 };

    filter_decimate_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_FACTOR, 1},
                           {PARAM_TYPE, SAMPLE_AVERAGE}},
                          initialParamValues)
    {
        this->gui = new filtergui_decimate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_decimate_c)

    void apply(image_s *const image) override;
    std::string uuid(void) const override { return "eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03"; }
    std::string name(void) const override { return "Decimate"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
