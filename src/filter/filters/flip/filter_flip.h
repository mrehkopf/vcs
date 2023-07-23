/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H
#define VCS_FILTER_FILTERS_FLIP_FILTER_FLIP_H

#include "filter/abstract_filter.h"
#include "filter/filters/flip/gui/filtergui_flip.h"

class filter_flip_c : public abstract_filter_c
{
public:
    enum { PARAM_AXIS };

    filter_flip_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({{PARAM_AXIS, 0}}, initialParamValues)
    {
        this->gui = new filtergui_flip_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_flip_c)

    std::string uuid(void) const override { return "80a3ac29-fcec-4ae0-ad9e-bbd8667cc680"; }
    std::string name(void) const override { return "Flip"; }
    filter_category_e category(void) const override { return filter_category_e::distort; }
    void apply(image_s *const image) override;

private:
};

#endif
