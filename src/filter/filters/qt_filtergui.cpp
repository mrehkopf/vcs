/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include <QDebug>
#include "filter/filters/qt_filtergui.h"

filtergui_c::filtergui_c(filter_c *const filter,
                         const unsigned minWidth) :
    filter(filter),
    title(QString::fromStdString(filter->name())),
    minWidth(minWidth)
{
    this->widget = new QFrame();
    this->widget->setMinimumWidth(this->minWidth);

    return;
}

filtergui_c::~filtergui_c(void)
{
    // Note: this->widget, which is allocated by this class's constructor, is
    // expected to be embedded into a QGraphicsProxyWidget by VCS's filter graph,
    // and so deleted automatically when the filter graph is destroyed.

    return;
}
