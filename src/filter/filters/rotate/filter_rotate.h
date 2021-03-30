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
    enum { PARAM_ROT,
           PARAM_SCALE };

    filter_rotate_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_SCALE, 1},
                  {PARAM_ROT, 0}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_rotate_c(this);
    }

    std::string uuid(void) const override { return "140c514d-a4b0-4882-abc6-b4e9e1ff4451"; }
    std::string name(void) const override { return "Rotate"; }
    filter_type_e type(void) const override { return filter_type_e::rotate; }
    filter_category_e category(void) const override { return filter_category_e::distort; }

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

private:
};

#endif
