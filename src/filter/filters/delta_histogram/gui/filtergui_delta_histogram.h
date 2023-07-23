/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DELTA_HISTOGRAM_GUI_FILTERGUI_DELTA_HISTOGRAM_H
#define VCS_FILTER_FILTERS_DELTA_HISTOGRAM_GUI_FILTERGUI_DELTA_HISTOGRAM_H

#include "common/abstract_gui.h"

class abstract_filter_c;

class filtergui_delta_histogram_c : public abstract_gui_s
{
public:
    filtergui_delta_histogram_c(abstract_filter_c *const filter);
};

#endif
