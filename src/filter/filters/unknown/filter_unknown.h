/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_UNKNOWN_FILTER_UNKNOWN_H
#define VCS_FILTER_FILTERS_UNKNOWN_FILTER_UNKNOWN_H

#include "filter/abstract_filter.h"

// Placeholder filter.
class filter_unknown_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_unknown_c)

    filter_unknown_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({}, initialParamValues)
    {
        this->gui = new abstract_gui_s();
    }

    void apply(image_s *const image) override
    {
        (void)image;
        return;
    }

    std::string uuid(void) const override { return "8de02db0-1114-4edc-a861-ce7665109deb"; }
    std::string name(void) const override { return "Unknown"; }
    filter_category_e category(void) const override { return filter_category_e::internal; }

private:
};

#endif
