/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H
#define VCS_FILTER_FILTERS_UNSHARP_MASK_FILTER_UNSHARP_MASK_H

#include "filter/abstract_filter.h"
#include "filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h"

class filter_unsharp_mask_c : public abstract_filter_c
{
public:
    enum { PARAM_STRENGTH,
           PARAM_RADIUS };

    filter_unsharp_mask_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_STRENGTH, 5},
                           {PARAM_RADIUS, 1}},
                          initialParamValues)
    {
        this->gui = new filtergui_unsharp_mask_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_unsharp_mask_c)

    std::string uuid(void) const override { return "03847778-bb9c-4e8c-96d5-0c10335c4f34"; }
    std::string name(void) const override { return "Unsharp mask"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }
    void apply(image_s *const image) override;

private:
};

#endif
