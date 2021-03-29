/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H
#define VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H

#include "filter/filter.h"
#include "filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h"

class filter_unsharp_mask_c : public filter_c
{
public:
    enum { PARAM_STRENGTH,
           PARAM_RADIUS };

    filter_unsharp_mask_c(const std::vector<std::pair<unsigned, double>> &initialParameterValues = {}) :
        filter_c({{PARAM_STRENGTH, 5},
                  {PARAM_RADIUS, 1}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_unsharp_mask_c(this);
    }

    std::string uuid(void) const override { return "03847778-bb9c-4e8c-96d5-0c10335c4f34"; }
    std::string name(void) const override { return "Unsharp Mask"; }
    filter_type_e type(void) const override { return filter_type_e::unsharp_mask; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
