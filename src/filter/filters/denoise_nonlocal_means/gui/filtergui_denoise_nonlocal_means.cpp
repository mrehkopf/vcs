/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"
#include "filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h"

filtergui_denoise_nonlocal_means_c::filtergui_denoise_nonlocal_means_c(abstract_filter_c *const filter)
{
    auto *luminance = filtergui::spinner(filter, filter_denoise_nonlocal_means_c::PARAM_H);
    luminance->minValue = 0;
    luminance->maxValue = 255;
    this->fields.push_back({"Luminance", {luminance}});

    auto *color = filtergui::spinner(filter, filter_denoise_nonlocal_means_c::PARAM_H_COLOR);
    color->minValue = 0;
    color->maxValue = 255;
    this->fields.push_back({"Color", {color}});

    auto *templateWindow = filtergui::spinner(filter, filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE);
    templateWindow->minValue = 0;
    templateWindow->maxValue = 255;
    this->fields.push_back({"Template wnd.", {templateWindow}});

    auto *searchWindow = filtergui::spinner(filter, filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE);
    searchWindow->minValue = 0;
    searchWindow->maxValue = 255;
    this->fields.push_back({"Search wnd.", {searchWindow}});
    
    return;
}
