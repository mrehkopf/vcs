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
    QLabel *hLabel = new QLabel("Luminance", this->widget);
    QLabel *hColorLabel = new QLabel("Color", this->widget);
    QLabel *templateWindowLabel = new QLabel("Template wnd.", this->widget);
    QLabel *searchWindowLabel = new QLabel("Search wnd.", this->widget);

    QSpinBox *hSpin = new QSpinBox(this->widget);
    hSpin->setRange(0, 255);
    hSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H));

    QSpinBox *hColorSpin = new QSpinBox(this->widget);
    hColorSpin->setRange(0, 255);
    hColorSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_H_COLOR));

    QSpinBox *templateWindowSpin = new QSpinBox(this->widget);
    templateWindowSpin->setRange(0, 255);
    templateWindowSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_TEMPLATE_WINDOW_SIZE));

    QSpinBox *searchWindowSpin = new QSpinBox(this->widget);
    searchWindowSpin->setRange(0, 255);
    searchWindowSpin->setValue(this->filter->parameter(filter_denoise_nonlocal_means_c::PARAM_SEARCH_WINDOW_SIZE));

    QFormLayout *layout = new QFormLayout(this->widget);
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

    this->widget->adjustSize();

    return;
}
