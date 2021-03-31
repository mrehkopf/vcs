/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CROP_FILTER_CROP_H
#define VCS_FILTER_FILTERS_CROP_FILTER_CROP_H

#include "filter/filter.h"
#include "filter/filters/crop/gui/filtergui_crop.h"

class filter_crop_c : public filter_c
{
public:
    enum { PARAM_X,
           PARAM_Y,
           PARAM_WIDTH,
           PARAM_HEIGHT,
           PARAM_SCALER };

    enum { SCALE_LINEAR,
           SCALE_NEAREST,
           SCALE_NONE };

    filter_crop_c(FILTER_CTOR_FUNCTION_PARAMS) :
        filter_c({{PARAM_X, 0},
                  {PARAM_Y, 0},
                  {PARAM_WIDTH, 640},
                  {PARAM_HEIGHT, 480},
                  {PARAM_SCALER, SCALE_NONE}},
                 initialParameterValues)
    {
        this->guiDescription = new filtergui_crop_c(this);
    }

    CLONABLE_FILTER_TYPE(filter_crop_c)

    void apply(FILTER_APPLY_FUNCTION_PARAMS) override;

    std::string uuid(void) const override { return "2448cf4a-112d-4d70-9fc1-b3e9176b6684"; }
    std::string name(void) const override { return "Crop"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
