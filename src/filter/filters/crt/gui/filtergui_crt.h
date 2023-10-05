/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CRT_FILTERGUI_CRT_H
#define VCS_FILTER_FILTERS_CRT_FILTERGUI_CRT_H

#include "common/abstract_gui.h"

class filtergui_crt_c : public abstract_gui_s
{
public:
    filtergui_crt_c(abstract_filter_c *const filter);
};

#endif
