/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FLIP_GUI_FILTERGUI_FLIP_H
#define VCS_FILTER_FILTERS_FLIP_GUI_FILTERGUI_FLIP_H

#include "filter/filters/qt_filtergui.h"

class filtergui_flip_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_flip_c(filter_c *const filter);
};

#endif
