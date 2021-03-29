/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filtergui.h"

filtergui_c::~filtergui_c(void)
{
    // Note: this->widget, which is allocated by this class's constructor, is
    // expected to be embedded into a QGraphicsProxyWidget by VCS's filter graph,
    // and so deleted automatically when the filter graph is destroyed.

    for (auto &row: this->guiFields)
    {
        for (auto *const component: row.components)
        {
            delete component;
        }
    }

    return;
}
