/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H
#define VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H

#include <chrono>
#include "filter/abstract_filter.h"
#include "filter/filters/frame_rate/gui/filtergui_frame_rate.h"

class filter_frame_rate_c : public abstract_filter_c
{
public:
    enum { PARAM_THRESHOLD,
           PARAM_CORNER,
           PARAM_BG_COLOR,
           PARAM_TEXT_COLOR,};

    enum { TOP_LEFT = 0,
           TOP_RIGHT = 1,
           BOTTOM_RIGHT = 2,
           BOTTOM_LEFT = 3 };

    enum { BG_TRANSPARENT = 0,
           BG_BLACK = 1,
           BG_WHITE = 2 };

    enum { TEXT_YELLOW = 0,
           TEXT_PURPLE = 1,
           TEXT_BLACK = 2,
           TEXT_WHITE = 3 };

    filter_frame_rate_c(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) :
        abstract_filter_c({{PARAM_THRESHOLD, 20},
                           {PARAM_CORNER, TOP_LEFT},
                           {PARAM_BG_COLOR, BG_TRANSPARENT},
                           {PARAM_TEXT_COLOR, TEXT_YELLOW}},
                          initialParamValues)
    {
        this->guiDescription = new filtergui_frame_rate_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_frame_rate_c)

    std::string uuid(void) const override { return "badb0129-f48c-4253-a66f-b0ec94e225a0"; }
    std::string name(void) const override { return "Frame rate"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

    void apply(u8 *const pixels, const resolution_s &r) override;

private:
};

#endif
