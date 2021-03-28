/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include "filter/filters/unsharp_mask/gui/qt/filtergui_unsharp_mask.h"

filtergui_unsharp_mask_c::filtergui_unsharp_mask_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Strength.
    QLabel *strLabel = new QLabel("Strength", frame);
    QSpinBox *strSpin = new QSpinBox(frame);
    strSpin->setRange(0, 255);
    strSpin->setValue(this->filter->parameter(filter_unsharp_mask_c::PARAM_STRENGTH));

    // Radius.
    QLabel *radiusLabel = new QLabel("Radius", frame);
    QSpinBox *radiusdSpin = new QSpinBox(frame);
    radiusdSpin->setRange(1, 255);
    radiusdSpin->setValue(this->filter->parameter(filter_unsharp_mask_c::PARAM_RADIUS) / 10);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(strLabel, strSpin);
    l->addRow(radiusLabel, radiusdSpin);

    connect(strSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_unsharp_mask_c::PARAM_STRENGTH, newValue);
    });

    connect(radiusdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_unsharp_mask_c::PARAM_RADIUS, (newValue * 10));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
