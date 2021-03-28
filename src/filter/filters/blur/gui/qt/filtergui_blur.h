/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H
#define VCS_FILTER_FILTERS_BLUR_GUI_FILTERGUI_BLUR_H

#include "filter/filters/qt_filtergui.h"

class filtergui_blur_c : public filtergui_c
{
    Q_OBJECT
    
public:
    filtergui_blur_c(filter_c *const filter);
};

#endif
