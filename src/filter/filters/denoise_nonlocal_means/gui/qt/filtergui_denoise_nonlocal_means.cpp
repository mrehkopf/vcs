/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"
#include "filter/filters/denoise_nonlocal_means/gui/qt/filtergui_denoise_nonlocal_means.h"

filtergui_denoise_nonlocal_means_c::filtergui_denoise_nonlocal_means_c(filter_c *const filter) : filtergui_c(filter)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *hLabel = new QLabel("Luminance", frame);
    QLabel *hColorLabel = new QLabel("Color", frame);
    QLabel *templateWindowLabel = new QLabel("Template wnd.", frame);
    QLabel *searchWindowLabel = new QLabel("Search wnd.", frame);

    QSpinBox *hSpin = new QSpinBox(frame);
    hSpin->setRange(0, 255);
    hSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H));

    QSpinBox *hColorSpin = new QSpinBox(frame);
    hColorSpin->setRange(0, 255);
    hColorSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H_COLOR));

    QSpinBox *templateWindowSpin = new QSpinBox(frame);
    templateWindowSpin->setRange(0, 255);
    templateWindowSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE));

    QSpinBox *searchWindowSpin = new QSpinBox(frame);
    searchWindowSpin->setRange(0, 255);
    searchWindowSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE));

    QFormLayout *layout = new QFormLayout(frame);
    layout->addRow(hColorLabel, hColorSpin);
    layout->addRow(hLabel, hSpin);
    layout->addRow(searchWindowLabel, searchWindowSpin);
    layout->addRow(templateWindowLabel, templateWindowSpin);

    connect(hSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_denoise_nonlocal_means_c::PARAM_H, newValue);
    });

    connect(hColorSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_denoise_nonlocal_means_c::PARAM_H_COLOR, newValue);
    });

    connect(templateWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE, newValue);
    });

    connect(searchWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
