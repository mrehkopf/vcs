/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_INPUT_GATE_GUI_FILTERGUI_INPUT_GATE_H
#define VCS_FILTER_FILTERS_INPUT_GATE_GUI_FILTERGUI_INPUT_GATE_H

#include "filter/filters/qt_filtergui.h"

class filtergui_input_gate_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_input_gate_c(filter_c *const filter);
};

#endif
