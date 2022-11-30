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

            auto *const spinbox = cols[col] = new filtergui_doublespinbox_s;
            spinbox->alignment = filtergui_alignment_e::center;
            spinbox->get_value = [=]{return filter->parameter(paramId);};
            spinbox->set_value = [=](const double value){filter->set_parameter(paramId, value);};
            spinbox->maxValue = 99;
            spinbox->minValue = -99;
            spinbox->numDecimals = 3;
        }

        this->guiFields.push_back({"", {cols[0], cols[1], cols[2]}});
    }

    return;
}
