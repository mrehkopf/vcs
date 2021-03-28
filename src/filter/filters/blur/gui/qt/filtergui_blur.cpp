/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/blur/filter_blur.h"
#include "filter/filters/blur/gui/qt/filtergui_blur.h"

filtergui_blur_c::filtergui_blur_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Blur type.
    QLabel *typeLabel = new QLabel("Type", frame);
    QComboBox *typeList = new QComboBox(frame);
    typeList->addItem("Box");
    typeList->addItem("Gaussian");
    typeList->setCurrentIndex(this->filter->parameter(filter_blur_c::PARAM_TYPE));

    // Blur radius.
    QLabel *radiusLabel = new QLabel("Radius", frame);
    QDoubleSpinBox *radiusSpin = new QDoubleSpinBox(frame);
    radiusSpin->setRange(0.1, 25);
    radiusSpin->setDecimals(1);
    radiusSpin->setValue(this->filter->parameter(filter_blur_c::PARAM_KERNEL_SIZE) / 10.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(typeLabel, typeList);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(filter_blur_c::PARAM_KERNEL_SIZE, std::round(newValue * 10.0));
    });

    connect(typeList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int currentIdx)
    {
        this->set_parameter(filter_blur_c::PARAM_TYPE, ((currentIdx < 0)? 0 : currentIdx));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
