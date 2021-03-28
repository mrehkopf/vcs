/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/frame_rate/filter_frame_rate.h"
#include "filter/filters/frame_rate/gui/qt/filtergui_frame_rate.h"

filtergui_frame_rate_c::filtergui_frame_rate_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold", frame);
    QSpinBox *thresholdSpin = new QSpinBox(frame);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->filter->parameter(filter_frame_rate_c::PARAM_THRESHOLD));

    // In which corner to show the counter.
    QLabel *cornerLabel = new QLabel("Show in", frame);
    QComboBox *cornerList = new QComboBox(frame);
    cornerList->addItem("Top left");
    cornerList->addItem("Top right");
    cornerList->addItem("Bottom right");
    cornerList->addItem("Bottom left");
    cornerList->setCurrentIndex(this->filter->parameter(filter_frame_rate_c::PARAM_CORNER));

    // Text color.
    QLabel *textColorLabel = new QLabel("Text", frame);
    QComboBox *textColorList = new QComboBox(frame);
    textColorList->addItem("Yellow");
    textColorList->addItem("Purple");
    textColorList->addItem("Black");
    textColorList->addItem("White");
    textColorList->setCurrentIndex(this->filter->parameter(filter_frame_rate_c::PARAM_TEXT_COLOR));

    // Background color.
    QLabel *bgColorLabel = new QLabel("Background", frame);
    QComboBox *bgColorList = new QComboBox(frame);
    bgColorList->addItem("Transparent");
    bgColorList->addItem("Black");
    bgColorList->addItem("White");
    bgColorList->setCurrentIndex(this->filter->parameter(filter_frame_rate_c::PARAM_BG_COLOR));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(cornerLabel, cornerList);
    l->addRow(textColorLabel, textColorList);
    l->addRow(bgColorLabel, bgColorList);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_frame_rate_c::PARAM_THRESHOLD, newValue);
    });

    connect(cornerList, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(filter_frame_rate_c::PARAM_CORNER, newIndex);
    });

    connect(bgColorList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(filter_frame_rate_c::PARAM_BG_COLOR, newIndex);
    });

    connect(textColorList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(filter_frame_rate_c::PARAM_TEXT_COLOR, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
