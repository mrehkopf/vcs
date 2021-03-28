/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/rotate/filter_rotate.h"
#include "filter/filters/rotate/gui/qt/filtergui_rotate.h"

filtergui_rotate_c::filtergui_rotate_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *rotLabel = new QLabel("Angle", frame);
    QDoubleSpinBox *rotSpin = new QDoubleSpinBox(frame);
    rotSpin->setDecimals(1);
    rotSpin->setRange(-360, 360);
    rotSpin->setValue(this->filter->parameter(filter_rotate_c::PARAM_ROT) / 10.0);

    QLabel *scaleLabel = new QLabel("Scale", frame);
    QDoubleSpinBox *scaleSpin = new QDoubleSpinBox(frame);
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(0.01, 20);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setValue(this->filter->parameter(filter_rotate_c::PARAM_SCALE) / 100.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(rotLabel, rotSpin);
    l->addRow(scaleLabel, scaleSpin);

    connect(rotSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(filter_rotate_c::PARAM_ROT, (newValue * 10));
    });

    connect(scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(filter_rotate_c::PARAM_SCALE, (newValue * 100));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
