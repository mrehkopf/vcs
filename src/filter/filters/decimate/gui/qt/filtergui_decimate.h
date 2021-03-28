/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DECIMATE_GUI_FILTERGUI_DECIMATE_H
#define VCS_FILTER_FILTERS_DECIMATE_GUI_FILTERGUI_DECIMATE_H

#include "filter/filters/qt_filtergui.h"

class filtergui_decimate_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_decimate_c(filter_c *const filter);
};

#endif
