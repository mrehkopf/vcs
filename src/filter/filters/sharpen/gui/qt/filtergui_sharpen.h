/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H
#define VCS_FILTER_FILTERS_SHARPEN_GUI_filtergui_sharpen_H

#include "filter/filters/qt_filtergui.h"

struct filtergui_sharpen_c : public filtergui_c
{
    filtergui_sharpen_c(filter_c *const filter);

private:
    Q_OBJECT
};

#endif
