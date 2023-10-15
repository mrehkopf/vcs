/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/rotate/filter_rotate.h"
#include "filter/filters/rotate/gui/filtergui_rotate.h"

filtergui_rotate_c::filtergui_rotate_c(abstract_filter_c *const filter)
{
    auto *angle = filtergui::double_spinner(filter, filter_rotate_c::PARAM_ROT);
    angle->minValue = -360;
    angle->maxValue = 360;
    angle->numDecimals = 2;
    this->fields.push_back({"Angle", {angle}});

    auto *scale = filtergui::double_spinner(filter, filter_rotate_c::PARAM_SCALE);
    scale->minValue = 0.01;
    scale->maxValue = 20;
    scale->numDecimals = 2;
    scale->stepSize = 0.1;
    this->fields.push_back({"Scale", {scale}});

    return;
}
