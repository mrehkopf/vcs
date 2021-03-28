/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/crop/filter_crop.h"
#include "filter/filters/crop/gui/qt/filtergui_crop.h"

filtergui_crop_c::filtergui_crop_c(filter_c *const filter) : filtergui_c(filter)
{
    QLabel *xLabel = new QLabel("X", this->widget);
    QSpinBox *xSpin = new QSpinBox(this->widget);
    xSpin->setRange(0, MAX_CAPTURE_WIDTH);
    xSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_X));

    QLabel *yLabel = new QLabel("Y", this->widget);
    QSpinBox *ySpin = new QSpinBox(this->widget);
    ySpin->setRange(0, MAX_CAPTURE_HEIGHT);
    ySpin->setValue(this->filter->parameter(filter_crop_c::PARAM_Y));

    QLabel *widthLabel = new QLabel("Width", this->widget);
    QSpinBox *widthSpin = new QSpinBox(this->widget);
    widthSpin->setRange(0, MAX_CAPTURE_WIDTH);
    widthSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", this->widget);
    QSpinBox *heightSpin = new QSpinBox(this->widget);
    heightSpin->setRange(0, MAX_CAPTURE_HEIGHT);
    heightSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_HEIGHT));

    QLabel *scalerLabel = new QLabel("Scaler", this->widget);
    QComboBox *scalerList = new QComboBox(this->widget);
    scalerList->addItem("Linear");
    scalerList->addItem("Nearest");
    scalerList->addItem("(Don't scale)");
    scalerList->setCurrentIndex(this->filter->parameter(filter_crop_c::PARAM_SCALER));

    QFormLayout *l = new QFormLayout(this->widget);
    l->addRow(xLabel, xSpin);
    l->addRow(yLabel, ySpin);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);
    l->addRow(scalerLabel, scalerList);

    connect(xSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_crop_c::PARAM_X, newValue);
    });

    connect(ySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_crop_c::PARAM_Y, newValue);
    });

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_crop_c::PARAM_WIDTH, newValue);
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_crop_c::PARAM_HEIGHT, newValue);
    });

    connect(scalerList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(filter_crop_c::PARAM_SCALER, newIndex);
    });

    this->widget->adjustSize();

    return;
}
