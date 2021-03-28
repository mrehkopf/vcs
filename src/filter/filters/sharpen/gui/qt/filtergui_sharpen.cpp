/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/sharpen/filter_sharpen.h"
#include "filter/filters/sharpen/gui/qt/filtergui_sharpen.h"

filtergui_sharpen_c::filtergui_sharpen_c(filter_c *const filter) : filtergui_c(filter)
{
    QLabel *noneLabel = new QLabel(this->noParamsMsg);
    noneLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout *l = new QHBoxLayout(this->widget);
    l->addWidget(noneLabel);

    this->widget->adjustSize();

    return;
}
