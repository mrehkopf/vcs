/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FRAME_RATE_GUI_FILTERGUI_FRAME_RATE_H
#define VCS_FILTER_FILTERS_FRAME_RATE_GUI_FILTERGUI_FRAME_RATE_H

#include "filter/filters/qt_filtergui.h"

struct filtergui_frame_rate_c : public filtergui_c
{
    filtergui_frame_rate_c(filter_c *const filter);

private:
    Q_OBJECT
};

#endif
