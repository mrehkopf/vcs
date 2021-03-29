/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_FILTER_KERNEL_3X3_H
#define VCS_FILTER_FILTERS_SHARPEN_FILTER_KERNEL_3X3_H

#include "filter/filter.h"
#include "filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h"

class filter_kernel_3x3_c : public filter_c
{
public:
    enum { PARAM_11,
           PARAM_12,
           PARAM_13,
           PARAM_21,
           PARAM_22,
           PARAM_23,
           PARAM_31,
           PARAM_32,
           PARAM_33,};

    filter_kernel_3x3_c(const std::vector<std::pair<unsigned, double>> &initialParameterValues = {}) :
        filter_c({{PARAM_11, 0},
                  {PARAM_12, 0},
                  {PARAM_13, 0},
                  {PARAM_21, 0},
                  {PARAM_22, 1},
                  {PARAM_23, 0},
                  {PARAM_31, 0},
                  {PARAM_32, 0},
                  {PARAM_33, 0}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_kernel_3x3_c(this);
    }

    std::string uuid(void) const override { return "95027807-978b-4371-9a14-f6166efc64d9"; }
    std::string name(void) const override { return "Kernel (3 \u00d7 3)"; }
    filter_type_e type(void) const override { return filter_type_e::kernel_3x3; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
