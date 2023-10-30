/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "display/qt/persistent_settings.h"
#include "capture/capture.h"
#include "CaptureRate.h"
#include "ui_CaptureRate.h"

control_panel::capture::CaptureRate::CaptureRate(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::CaptureRate)
{
    ui->setupUi(this);
    this->set_name("Capture rate");

    this->ui->doubleSpinBox_captureRate->setValue(60);
    this->ui->doubleSpinBox_captureRate->setMinimum(1);
    this->ui->doubleSpinBox_captureRate->setMaximum(1000);
    this->ui->doubleSpinBox_captureRate->setDecimals(refresh_rate_s::numDecimalsPrecision);

    connect(this->ui->groupBox, &QGroupBox::toggled, this, [this](const bool isChecked)
    {
        kpers_set_value(INI_GROUP_CAPTURE, "ForceRateEnabled", isChecked);
        capture_rate_s::to_capture_device_properties(
            isChecked
                ? this->ui->doubleSpinBox_captureRate->value()
                : refresh_rate_s::from_capture_device_properties()
        );
    });

    connect(this->ui->doubleSpinBox_captureRate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [this]
    {
        const double val = this->ui->doubleSpinBox_captureRate->value();
        kpers_set_value(INI_GROUP_CAPTURE, "ForceRate", val);

        if (this->ui->groupBox->isChecked())
        {
            capture_rate_s::to_capture_device_properties(refresh_rate_s(val));
        }
    });

    // Restore persistent settings.
    this->ui->doubleSpinBox_captureRate->setValue(kpers_value_of(INI_GROUP_CAPTURE, "ForceRate", 60).toDouble());
    this->ui->groupBox->setChecked(kpers_value_of(INI_GROUP_CAPTURE, "ForceRateEnabled", false).toBool());

    // Listen for app events.
    {
        ev_capture_signal_lost.listen([this]
        {
            this->ui->groupBox->setEnabled(false);
        });

        ev_capture_signal_gained.listen([this]
        {
            this->ui->groupBox->setEnabled(true);
        });

        ev_new_video_mode.listen([this](const video_mode_s &mode)
        {
            capture_rate_s::to_capture_device_properties(
                this->ui->groupBox->isChecked()
                    ? this->ui->doubleSpinBox_captureRate->value()
                    : mode.refreshRate
            );
        });
    }

    return;
}

control_panel::capture::CaptureRate::~CaptureRate()
{
    delete ui;

    return;
}
