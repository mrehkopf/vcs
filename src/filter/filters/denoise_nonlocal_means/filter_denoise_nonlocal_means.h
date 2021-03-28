/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_FILTER_DENOISE_NONLOCAL_MEANS_H
#define VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_FILTER_DENOISE_NONLOCAL_MEANS_H

#include "filter/filter.h"
#include "filter/filters/denoise_nonlocal_means/gui/qt/filtergui_denoise_nonlocal_means.h"

class filter_denoise_nonlocal_means_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_H = 0,
                              PARAM_H_COLOR = 1,
                              PARAM_TEMPLATE_WINDOW_SIZE = 2,
                              PARAM_SEARCH_WINDOW_SIZE = 3};

    filter_denoise_nonlocal_means_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_H>(10),
                  filter_c::make_parameter<u8, PARAM_H_COLOR>(10),
                  filter_c::make_parameter<u8, PARAM_TEMPLATE_WINDOW_SIZE>(7),
                  filter_c::make_parameter<u8, PARAM_SEARCH_WINDOW_SIZE>(21)})
    {
        this->guiWidget = new filtergui_denoise_nonlocal_means_c(this);
    }

    std::string uuid(void) const override { return "e31d5ee3-f5df-4e7c-81b8-227fc39cbe76"; }
    std::string name(void) const override { return "Denoise (non-local means)"; }
    filter_type_e type(void) const override { return filter_type_e::denoise_nonlocal_means; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
