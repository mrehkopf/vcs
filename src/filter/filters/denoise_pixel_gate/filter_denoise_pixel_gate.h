/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H
#define VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H

#include "filter/filter.h"
#include "filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h"

class filter_denoise_pixel_gate_c : public filter_c
{
public:
    enum { PARAM_THRESHOLD };
                         
    filter_denoise_pixel_gate_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_THRESHOLD, 5}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_denoise_pixel_gate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_denoise_pixel_gate_c)

    std::string uuid(void) const override { return "94adffac-be42-43ac-9839-9cc53a6d615c"; }
    std::string name(void) const override { return "Denoise (pixel gate)"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

private:
};

#endif
