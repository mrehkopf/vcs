/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DELTA_HISTGRAM_FILTER_DELTA_HISTOGRAM_H
#define VCS_FILTER_FILTERS_DELTA_HISTGRAM_FILTER_DELTA_HISTOGRAM_H

#include "filter/abstract_filter.h"
#include "filter/filters/delta_histogram/gui/filtergui_delta_histogram.h"

class filter_delta_histogram_c : public abstract_filter_c
{
public:
    filter_delta_histogram_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({}, initialParamValues)
    {
        this->gui = new filtergui_delta_histogram_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_delta_histogram_c)

    void apply(image_s *const image) override;
    std::string uuid(void) const override { return "fc85a109-c57a-4317-994f-786652231773"; }
    std::string name(void) const override { return "Histogram: pixel delta"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

private:
};

#endif
