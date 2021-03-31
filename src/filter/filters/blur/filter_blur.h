/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_BLUR_FILTER_BLUR_H
#define VCS_FILTER_FILTERS_BLUR_FILTER_BLUR_H

#include "filter/filter.h"
#include "filter/filters/blur/gui/filtergui_blur.h"

class filter_blur_c : public filter_c
{
public:
    enum { PARAM_TYPE,
           PARAM_KERNEL_SIZE };

    enum { BLUR_BOX = 0,
           BLUR_GAUSSIAN = 1 };
    
    filter_blur_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_KERNEL_SIZE, 1},
                  {PARAM_TYPE, BLUR_GAUSSIAN}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_blur_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_blur_c)

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

    std::string name(void) const override { return "Blur"; }
    std::string uuid(void) const override { return "a5426f2e-b060-48a9-adf8-1646a2d3bd41"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif