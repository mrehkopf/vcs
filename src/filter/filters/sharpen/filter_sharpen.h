/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_FILTER_SHARPEN_H
#define VCS_FILTER_FILTERS_SHARPEN_FILTER_SHARPEN_H

#include "filter/abstract_filter.h"
#include "filter/filters/sharpen/gui/filtergui_sharpen.h"

class filter_sharpen_c : public abstract_filter_c
{
public:
    filter_sharpen_c(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) :
        abstract_filter_c({}, initialParamValues)
    {
        this->guiDescription = new filtergui_sharpen_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_sharpen_c)

    std::string uuid(void) const override { return "1c25bbb1-dbf4-4a03-93a1-adf24b311070"; }
    std::string name(void) const override { return "Sharpen"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(u8 *const pixels, const resolution_s &r) override;

private:
};

#endif
