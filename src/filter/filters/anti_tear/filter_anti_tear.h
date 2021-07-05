/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ANTI_TEAR_FILTER_ANTI_TEAR_H
#define VCS_FILTER_FILTERS_ANTI_TEAR_FILTER_ANTI_TEAR_H

#include "anti_tear/anti_tear.h"
#include "filter/abstract_filter.h"
#include "filter/filters/anti_tear/gui/filtergui_anti_tear.h"

class filter_anti_tear_c : public abstract_filter_c
{
public:
    enum { PARAM_SCAN_DIRECTION,
           PARAM_SCAN_HINT,
           PARAM_SCAN_START,
           PARAM_SCAN_END,
           PARAM_THRESHOLD,
           PARAM_WINDOW_LENGTH,
           PARAM_STEP_SIZE,
           PARAM_MATCHES_REQD,
           PARAM_VISUALIZE_TEARS,
           PARAM_VISUALIZE_RANGE };

    enum { SCAN_DOWN = 0,
           SCAN_UP = 1, };

    enum { SCAN_ONE_TEAR = 0,
           SCAN_MULTIPLE_TEARS = 1 };

    filter_anti_tear_c(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) :
        abstract_filter_c({{PARAM_SCAN_DIRECTION, SCAN_DOWN},
                           {PARAM_SCAN_HINT, SCAN_MULTIPLE_TEARS},
                           {PARAM_SCAN_START, 0},
                           {PARAM_SCAN_END, 0},
                           {PARAM_THRESHOLD, KAT_DEFAULT_THRESHOLD},
                           {PARAM_WINDOW_LENGTH, KAT_DEFAULT_WINDOW_LENGTH},
                           {PARAM_STEP_SIZE, KAT_DEFAULT_STEP_SIZE},
                           {PARAM_MATCHES_REQD, KAT_DEFAULT_NUM_MATCHES_REQUIRED},
                           {PARAM_VISUALIZE_TEARS, 0},
                           {PARAM_VISUALIZE_RANGE, 0}},
                          initialParamValues)
    {
        this->guiDescription = new filtergui_anti_tear_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_anti_tear_c)

    std::string uuid(void) const override { return "11c27e0a-a000-41e9-a134-7579073c7dc5"; }
    std::string name(void) const override { return "Anti-tear"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

    void apply(u8 *const pixels, const resolution_s &r) override;

private:
};

#endif
