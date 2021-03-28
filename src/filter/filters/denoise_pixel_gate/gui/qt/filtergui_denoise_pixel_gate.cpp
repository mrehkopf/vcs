/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "filter/filters/denoise_pixel_gate/gui/qt/filtergui_denoise_pixel_gate.h"

filtergui_denoise_pixel_gate_c::filtergui_denoise_pixel_gate_c(filter_c *const filter) : filtergui_c(filter)
{
    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold", this->widget);
    QSpinBox *thresholdSpin = new QSpinBox(this->widget);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->filter->parameter(filter_denoise_pixel_gate_c::PARAM_THRESHOLD));

    QFormLayout *l = new QFormLayout(this->widget);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_denoise_pixel_gate_c::PARAM_THRESHOLD, newValue);
    });

    this->widget->adjustSize();

    return;
}
