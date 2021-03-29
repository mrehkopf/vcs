/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ROTATE_FILTER_ROTATE_H
#define VCS_FILTER_FILTERS_ROTATE_FILTER_ROTATE_H

#include "filter/filter.h"
#include "filter/filters/rotate/gui/filtergui_rotate.h"

class filter_rotate_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_ROT = 0,
                              PARAM_SCALE = 2 };

    filter_rotate_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u16, PARAM_SCALE>(100),
                  filter_c::make_parameter<u16, PARAM_ROT>(0)})
    {
        this->guiDescription = new filtergui_rotate_c(this);
    }

    std::string uuid(void) const override { return "140c514d-a4b0-4882-abc6-b4e9e1ff4451"; }
    std::string name(void) const override { return "Rotate"; }
    filter_type_e type(void) const override { return filter_type_e::rotate; }
    filter_category_e category(void) const override { return filter_category_e::distort; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
