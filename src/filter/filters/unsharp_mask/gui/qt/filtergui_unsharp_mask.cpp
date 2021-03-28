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
    // Strength.
    QLabel *strLabel = new QLabel("Strength", this->widget);
    QSpinBox *strSpin = new QSpinBox(this->widget);
    strSpin->setRange(0, 255);
    strSpin->setValue(this->filter->parameter(filter_unsharp_mask_c::PARAM_STRENGTH));

    // Radius.
    QLabel *radiusLabel = new QLabel("Radius", this->widget);
    QSpinBox *radiusSpin = new QSpinBox(this->widget);
    radiusSpin->setRange(1, 255);
    radiusSpin->setValue(this->filter->parameter(filter_unsharp_mask_c::PARAM_RADIUS) / 10);

    QFormLayout *l = new QFormLayout(this->widget);
    l->addRow(strLabel, strSpin);
    l->addRow(radiusLabel, radiusSpin);

    connect(strSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_unsharp_mask_c::PARAM_STRENGTH, newValue);
    });

    connect(radiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_unsharp_mask_c::PARAM_RADIUS, (newValue * 10));
    });

    this->widget->adjustSize();
    
    return;
}
