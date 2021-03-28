/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CROP_FILTER_CROP_H
#define VCS_FILTER_FILTERS_CROP_FILTER_CROP_H

#include "filter/filter.h"
#include "filter/filters/crop/gui/qt/filtergui_crop.h"

class filter_crop_c : public filter_c
{
public:
    enum parameter_offset_e { PARAM_X = 0,
                              PARAM_Y = 2,
                              PARAM_WIDTH = 4,
                              PARAM_HEIGHT = 6,
                              PARAM_SCALER = 8 };

    filter_crop_c(const u8 *const initialParameterArray = nullptr) :
        filter_c(initialParameterArray,
                 {filter_c::make_parameter<u16, PARAM_X>(0),
                  filter_c::make_parameter<u16, PARAM_Y>(0),
                  filter_c::make_parameter<u16, PARAM_WIDTH>(640),
                  filter_c::make_parameter<u16, PARAM_HEIGHT>(480),
                  filter_c::make_parameter<u8,  PARAM_SCALER>(0)})
    {
        this->guiWidget = new filtergui_crop_c(this);
    }

    void apply(FILTER_FUNC_PARAMS) const override;

    std::string uuid(void) const override { return "2448cf4a-112d-4d70-9fc1-b3e9176b6684"; }
    std::string name(void) const override { return "Crop"; }
    filter_type_e type(void) const override { return filter_type_e::crop; }
    filter_category_e category(void) const override { return filter_category_e::distort; }

private:
};

#endif
