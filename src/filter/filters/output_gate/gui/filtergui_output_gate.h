/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_GATE_GUI_FILTERGUI_OUTPUT_GATE_H
#define VCS_FILTER_FILTERS_OUTPUT_GATE_GUI_FILTERGUI_OUTPUT_GATE_H

#include "filter/filtergui.h"

class filtergui_output_gate_c : public filtergui_c
{
public:
    filtergui_output_gate_c(abstract_filter_c *const filter);
};

#endif
