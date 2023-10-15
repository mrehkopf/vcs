/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/flip/filter_flip.h"
#include "filter/filters/flip/gui/filtergui_flip.h"

filtergui_flip_c::filtergui_flip_c(abstract_filter_c *const filter)
{
    auto *axis = filtergui::combo_box(filter, filter_flip_c::PARAM_AXIS);
    axis->items = {"Vertical", "Horizontal", "Both"};
    this->fields.push_back({"Axis", {axis}});

    return;
}
