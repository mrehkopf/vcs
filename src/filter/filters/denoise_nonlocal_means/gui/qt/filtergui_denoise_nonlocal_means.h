/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_GUI_FILTERGUI_DENOISE_NONLOCAL_MEANS_H
#define VCS_FILTER_FILTERS_DENOISE_NONLOCAL_MEANS_GUI_FILTERGUI_DENOISE_NONLOCAL_MEANS_H

#include "filter/filters/qt_filtergui.h"

struct filtergui_denoise_nonlocal_means_c : public filtergui_c
{
    filtergui_denoise_nonlocal_means_c(filter_c *const filter);

private:
    Q_OBJECT
};

#endif
