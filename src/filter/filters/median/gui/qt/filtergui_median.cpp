/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/median/filter_median.h"
#include "filter/filters/median/gui/qt/filtergui_median.h"

filtergui_median_c::filtergui_median_c(filter_c *const filter) : filtergui_c(filter)
{
    // Median radius.
    QLabel *radiusLabel = new QLabel("Radius", this->widget);
    QSpinBox *radiusSpin = new QSpinBox(this->widget);
    radiusSpin->setRange(0, 99);
    radiusSpin->setValue(this->filter->parameter(filter_median_c::PARAM_KERNEL_SIZE) / 2);

    QFormLayout *l = new QFormLayout(this->widget);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_median_c::PARAM_KERNEL_SIZE, ((newValue * 2) + 1));
    });

    this->widget->adjustSize();

    return;
}
