/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_OUTPUT_SCALER_GUI_FILTERGUI_OUTPUT_SCALER_H
#define VCS_FILTER_FILTERS_OUTPUT_SCALER_GUI_FILTERGUI_OUTPUT_SCALER_H

#include "common/abstract_gui.h"

class filtergui_output_scaler_c : public abstract_gui_s
{
public:
    filtergui_output_scaler_c(abstract_filter_c *const filter);
};

#endif
