/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DELTA_HISTOGRAM_GUI_FILTERGUI_DELTA_HISTOGRAM_H
#define VCS_FILTER_FILTERS_DELTA_HISTOGRAM_GUI_FILTERGUI_DELTA_HISTOGRAM_H

#include "filter/filters/qt_filtergui.h"

struct filtergui_delta_histogram_c : public filtergui_c
{
    filtergui_delta_histogram_c(filter_c *const filter);

private:
    Q_OBJECT
};

#endif
