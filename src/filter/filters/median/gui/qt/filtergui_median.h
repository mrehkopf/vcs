/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_MEDIAN_GUI_FILTERGUI_MEDIAN_H
#define VCS_FILTER_FILTERS_MEDIAN_GUI_FILTERGUI_MEDIAN_H

#include "filter/filters/qt_filtergui.h"

class filtergui_median_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_median_c(filter_c *const filter);
};

#endif
