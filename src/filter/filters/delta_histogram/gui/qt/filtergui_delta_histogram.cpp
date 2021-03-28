/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/delta_histogram/gui/qt/filtergui_delta_histogram.h"

filtergui_delta_histogram_c::filtergui_delta_histogram_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *noneLabel = new QLabel(this->noParamsMsg);
    noneLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout *l = new QHBoxLayout(frame);
    l->addWidget(noneLabel);

    frame->adjustSize();
    this->widget = frame;

    return;
}
