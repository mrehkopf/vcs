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
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *xLabel = new QLabel("X", frame);
    QSpinBox *xSpin = new QSpinBox(frame);
    xSpin->setRange(0, MAX_CAPTURE_WIDTH);
    xSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_X));

    QLabel *yLabel = new QLabel("Y", frame);
    QSpinBox *ySpin = new QSpinBox(frame);
    ySpin->setRange(0, MAX_CAPTURE_HEIGHT);
    ySpin->setValue(this->filter->parameter(filter_crop_c::PARAM_Y));

    QLabel *widthLabel = new QLabel("Width", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, MAX_CAPTURE_WIDTH);
    widthSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, MAX_CAPTURE_HEIGHT);
    heightSpin->setValue(this->filter->parameter(filter_crop_c::PARAM_HEIGHT));

    QLabel *scalerLabel = new QLabel("Scaler", frame);
    QComboBox *scalerList = new QComboBox(frame);
    scalerList->addItem("Linear");
    scalerList->addItem("Nearest");
    scalerList->addItem("(Don't scale)");
    scalerList->setCurrentIndex(this->filter->parameter(filter_crop_c::PARAM_SCALER));

    QFormLayout *l = new QFormLayout(frame);
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

    frame->adjustSize();
    this->widget = frame;

    return;
}
