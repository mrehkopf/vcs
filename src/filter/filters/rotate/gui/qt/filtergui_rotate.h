/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ROTATE_GUI_FILTERGUI_ROTATE_H
#define VCS_FILTER_FILTERS_ROTATE_GUI_FILTERGUI_ROTATE_H

#include "filter/filters/qt_filtergui.h"

struct filtergui_rotate_c : public filtergui_c
{
    filtergui_rotate_c(filter_c *const filter);

private:
    Q_OBJECT
};

#endif
