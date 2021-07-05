/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/kernel_3x3/filter_kernel_3x3.h"
#include "filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h"

filtergui_kernel_3x3_c::filtergui_kernel_3x3_c(abstract_filter_c *const filter)
{
    for (unsigned row = 0; row < 3; row++)
    {
        filtergui_doublespinbox_s *cols[3];

        for (unsigned col = 0; col < 3; col++)
        {
            const unsigned paramId = ((row * 3) + col);

            cols[col] = new filtergui_doublespinbox_s;

            cols[col]->get_value = [=]{return filter->parameter(paramId);};
            cols[col]->set_value = [=](const double value){filter->set_parameter(paramId, value);};
            cols[col]->maxValue = 99;
            cols[col]->minValue = -99;
            cols[col]->numDecimals = 3;
        }

        this->guiFields.push_back({"", {cols[0], cols[1], cols[2]}});
    }

    return;
}
