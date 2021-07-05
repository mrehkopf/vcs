/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_MEDIAN_FILTER_MEDIAN_H
#define VCS_FILTER_FILTERS_MEDIAN_FILTER_MEDIAN_H

#include "filter/abstract_filter.h"
#include "filter/filters/median/gui/filtergui_median.h"

class filter_median_c : public abstract_filter_c
{
public:
    enum { PARAM_KERNEL_SIZE };

    filter_median_c(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) :
        abstract_filter_c({{PARAM_KERNEL_SIZE, 3}}, initialParamValues)
    {
        this->guiDescription = new filtergui_median_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_median_c)

    std::string uuid(void) const override { return "de60017c-afe5-4e5e-99ca-aca5756da0e8"; }
    std::string name(void) const override { return "Median"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

    void apply(u8 *const pixels, const resolution_s &r) override;

private:
};

#endif
