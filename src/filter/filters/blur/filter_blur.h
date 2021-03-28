/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_BLUR_FILTER_BLUR_H
#define VCS_FILTER_FILTERS_BLUR_FILTER_BLUR_H

#include "filter/filter.h"
#include "filter/filters/blur/gui/qt/filtergui_blur.h"

class filter_blur_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_TYPE = 0, PARAM_KERNEL_SIZE = 1 };
    enum { BLUR_BOX = 0, BLUR_GAUSSIAN = 1 };
    
    filter_blur_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_KERNEL_SIZE>(10),
                  filter_c::make_parameter<u8, PARAM_TYPE>(BLUR_GAUSSIAN)})
    {
        this->guiWidget = new filtergui_blur_c(this);
    }

    void apply(FILTER_FUNC_PARAMS) const override;

    std::string name(void) const override { return "Blur"; }
    std::string uuid(void) const override { return "a5426f2e-b060-48a9-adf8-1646a2d3bd41"; }
    filter_type_e type(void) const override { return filter_type_e::blur; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
