/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H
#define VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H

#include "filter/filter.h"
#include "filter/filters/flip/gui/filtergui_flip.h"

class filter_flip_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_AXIS = 0 };

    filter_flip_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_AXIS>(0)})
    {
        this->guiDescription = new filtergui_flip_c(this);
    }

    std::string uuid(void) const override { return "80a3ac29-fcec-4ae0-ad9e-bbd8667cc680"; }
    std::string name(void) const override { return "Flip"; }
    filter_type_e type(void) const override { return filter_type_e::flip; }
    filter_category_e category(void) const override { return filter_category_e::distort; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
