/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H
#define VCS_FILTER_FILTERS_FRAME_RATE_FILTER_FRAME_RATE_H

#include "filter/filter.h"
#include "filter/filters/frame_rate/gui/filtergui_frame_rate.h"

class filter_frame_rate_c : public filter_c
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

    filter_frame_rate_c(const std::vector<std::pair<unsigned, double>> &initialParameterValues = {}) :
        filter_c({{PARAM_THRESHOLD, 20},
                  {PARAM_CORNER, 0},
                  {PARAM_BG_COLOR, 0},
                  {PARAM_TEXT_COLOR, 0}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_frame_rate_c(this);
    }

    std::string uuid(void) const override { return "badb0129-f48c-4253-a66f-b0ec94e225a0"; }
    std::string name(void) const override { return "Frame rate"; }
    filter_type_e type(void) const override { return filter_type_e::frame_rate; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

    void apply(FILTER_FUNC_PARAMS) const override;

private:
};

#endif
