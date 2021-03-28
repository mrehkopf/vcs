/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/input_gate/filter_input_gate.h"
#include "filter/filters/input_gate/gui/qt/filtergui_input_gate.h"

filtergui_input_gate_c::filtergui_input_gate_c(filter_c *const filter) : filtergui_c(filter, 180)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *widthLabel = new QLabel("Width", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, MAX_CAPTURE_WIDTH);
    widthSpin->setValue(this->filter->parameter(filter_input_gate_c::PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, MAX_CAPTURE_WIDTH);
    heightSpin->setValue(this->filter->parameter(filter_input_gate_c::PARAM_HEIGHT));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_input_gate_c::PARAM_WIDTH, newValue);
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_input_gate_c::PARAM_HEIGHT, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
