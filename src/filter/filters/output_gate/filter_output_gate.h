/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H
#define VCS_FILTER_FILTERS_OUTPUT_GATE_FILTER_OUTPUT_GATE_H

#include "filter/filter.h"
#include "filter/filters/output_gate/gui/filtergui_output_gate.h"

class filter_output_gate_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_WIDTH = 0, PARAM_HEIGHT = 2};

    filter_output_gate_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u16, PARAM_WIDTH>(640),
                  filter_c::make_parameter<u16, PARAM_HEIGHT>(480)})
    {
        this->guiDescription = new filtergui_output_gate_c(this);
    }

    std::string uuid(void) const override { return "be8443e2-4355-40fd-aded-63cebcbfb8ce"; }
    std::string name(void) const override { return "Output gate"; }
    filter_type_e type(void) const override { return filter_type_e::output_gate; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
