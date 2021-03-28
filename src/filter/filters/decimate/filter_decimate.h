/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H
#define VCS_FILTER_FILTERS_DECIMATE_FILTER_DECIMATE_H

#include "filter/filter.h"
#include "filter/filters/decimate/gui/qt/filtergui_decimate.h"

class filter_decimate_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_TYPE = 0, PARAM_FACTOR = 1 };
    enum { SAMPLE_NEAREST = 0, SAMPLE_AVERAGE = 1 };

    filter_decimate_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_FACTOR>(2),
                  filter_c::make_parameter<u8, PARAM_TYPE>(SAMPLE_AVERAGE)})
    {
        this->guiWidget = new filtergui_decimate_c(this);
    }

    void apply(FILTER_FUNC_PARAMS) const override;

    std::string uuid(void) const override { return "eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03"; }
    std::string name(void) const override { return "Decimate"; }
    filter_type_e type(void) const override { return filter_type_e::decimate; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
