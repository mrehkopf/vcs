/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_INPUT_GATE_FILTER_INPUT_GATE_H
#define VCS_FILTER_FILTERS_INPUT_GATE_FILTER_INPUT_GATE_H

#include "filter/filter.h"
#include "filter/filters/input_gate/gui/filtergui_input_gate.h"

class filter_input_gate_c : public filter_c
{
public:
    enum { PARAM_WIDTH,
           PARAM_HEIGHT };

    filter_input_gate_c(const std::vector<std::pair<unsigned, double>> &initialParameterValues = {}) :
        filter_c({{PARAM_WIDTH, 640},
                  {PARAM_HEIGHT, 480}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_input_gate_c(this);
    }

    std::string uuid(void) const override { return "136deb34-ac79-46b1-a09c-d57dcfaa84ad"; }
    std::string name(void) const override { return "Input gate"; }
    filter_type_e type(void) const override { return filter_type_e::input_gate; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
