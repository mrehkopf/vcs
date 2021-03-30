/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_FILTER_DENOISE_NONLOCAL_MEANS_H
#define VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_FILTER_DENOISE_NONLOCAL_MEANS_H

#include "filter/filter.h"
#include "filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h"

class filter_denoise_nonlocal_means_c : public filter_c
{
public:
    enum { PARAM_H,
           PARAM_H_COLOR,
           PARAM_TEMPLATE_WINDOW_SIZE,
           PARAM_SEARCH_WINDOW_SIZE};

    filter_denoise_nonlocal_means_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_H, 10},
                  {PARAM_H_COLOR, 10},
                  {PARAM_TEMPLATE_WINDOW_SIZE, 7},
                  {PARAM_SEARCH_WINDOW_SIZE, 21}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_denoise_nonlocal_means_c(this);
    }

    std::string uuid(void) const override { return "e31d5ee3-f5df-4e7c-81b8-227fc39cbe76"; }
    std::string name(void) const override { return "Denoise (non-local means)"; }
    filter_type_e type(void) const override { return filter_type_e::denoise_nonlocal_means; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

private:
};

#endif
