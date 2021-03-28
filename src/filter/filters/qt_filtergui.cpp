/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/qt_filtergui.h"

filtergui_c::filtergui_c(filter_c *const filter,
                         const unsigned minWidth) :
    filter(filter),
    title(QString::fromStdString(filter->name())),
    minWidth(minWidth)
{
    return;
}

filtergui_c::~filtergui_c(void)
{
    return;
}
