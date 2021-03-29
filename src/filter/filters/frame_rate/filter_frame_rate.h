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
    enum parameter_offset_e { PARAM_THRESHOLD = 0,
                              PARAM_CORNER = 1,
                              PARAM_BG_COLOR = 2,
                              PARAM_TEXT_COLOR = 3,};

    filter_frame_rate_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u8, PARAM_THRESHOLD>(20),
                  filter_c::make_parameter<u8, PARAM_CORNER>(0),
                  filter_c::make_parameter<u8, PARAM_BG_COLOR>(0),
                  filter_c::make_parameter<u8, PARAM_TEXT_COLOR>(0)})
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
