/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H
#define VCS_FILTER_FILTERS_DENOISE_PIXEL_GATE_FILTER_DENOISE_PIXEL_GATE_H

#include "filter/filter.h"
#include "filter/filters/denoise_pixel_gate/gui/qt/filtergui_denoise_pixel_gate.h"

class filter_denoise_pixel_gate_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_THRESHOLD = 0};
                         
    filter_denoise_pixel_gate_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_THRESHOLD>(5)})
    {
        this->guiWidget = new filtergui_denoise_pixel_gate_c(this);
    }

    std::string uuid(void) const override { return "94adffac-be42-43ac-9839-9cc53a6d615c"; }
    std::string name(void) const override { return "Denoise (pixel gate)"; }
    filter_type_e type(void) const override { return filter_type_e::denoise_pixel_gate; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
