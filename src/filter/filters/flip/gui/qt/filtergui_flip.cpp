/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/flip/filter_flip.h"
#include "filter/filters/flip/gui/qt/filtergui_flip.h"

filtergui_flip_c::filtergui_flip_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *axisLabel = new QLabel("Axis", frame);
    QComboBox *axisList = new QComboBox(frame);
    axisList->addItem("Vertical");
    axisList->addItem("Horizontal");
    axisList->addItem("Both");
    axisList->setCurrentIndex(this->filter->parameter(filter_flip_c::PARAM_AXIS));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(axisLabel, axisList);

    connect(axisList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(filter_flip_c::PARAM_AXIS, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
