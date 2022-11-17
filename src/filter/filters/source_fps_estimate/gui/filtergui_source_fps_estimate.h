/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FRAME_RATE_GUI_FILTERGUI_FRAME_RATE_H
#define VCS_FILTER_FILTERS_FRAME_RATE_GUI_FILTERGUI_FRAME_RATE_H

#include "filter/filtergui.h"

class filtergui_source_fps_estimate_c : public filtergui_c
{
public:
    filtergui_source_fps_estimate_c(abstract_filter_c *const filter);
};

#endif
