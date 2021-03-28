/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CROP_GUI_FILTERGUI_CROP_H
#define VCS_FILTER_FILTERS_CROP_GUI_FILTERGUI_CROP_H

#include "filter/filters/qt_filtergui.h"

class filtergui_crop_c : public filtergui_c
{
    Q_OBJECT

public:
    filtergui_crop_c(filter_c *const filter);
};

#endif
