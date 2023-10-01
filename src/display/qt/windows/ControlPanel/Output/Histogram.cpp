/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QPainter>
#include "display/qt/widgets/ColorHistogram.h"
#include "display/qt/persistent_settings.h"
#include "common/vcs_event/vcs_event.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "Histogram.h"
#include "ui_Histogram.h"
#include <opencv2/imgproc/imgproc.hpp>

control_panel::output::Histogram::Histogram(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Histogram)
{
    this->ui->setupUi(this);

    // Wire up the GUI controls to consequences for operating them.
    {
        connect(this, &DialogFragment::enabled_state_set, this, [this](const bool isEnabled)
        {
            kpers_set_value(INI_GROUP_OUTPUT, "HistogramEnabled", isEnabled);

            if (!isEnabled)
            {
                this->ui->histogram->clear();
            }
            else
            {
                this->ui->histogram->refresh(ks_scaler_frame_buffer());
            }
        });

        connect(this->ui->groupBox, &QGroupBox::toggled, this, [this](const bool isChecked)
        {
            this->set_enabled(isChecked);
        });
    }

    // Listen for app events.
    {
        ev_new_output_image.listen([this](const image_s &image)
        {
            if (
                this->isVisible() &&
                this->ui->groupBox->isChecked()
            ){
                this->ui->histogram->refresh(image);
            }
        });

        ev_capture_signal_gained.listen([this]
        {
            this->ui->groupBox->setEnabled(true);
        });

        ev_capture_signal_lost.listen([this]
        {
            if (
                this->isVisible() &&
                this->ui->groupBox->isChecked()
            ){
                this->ui->histogram->clear();
            }

            this->ui->groupBox->setEnabled(false);
        });
    }

    // Restore persistent settings.
    {
        this->ui->groupBox->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "HistogramEnabled", true).toBool());
    }

    // Initialize GUI controls to their starting values.
    {
        if (this->ui->groupBox->isChecked())
        {
            this->ui->histogram->refresh(ks_scaler_frame_buffer());
        }
        else
        {
            this->ui->histogram->clear();
        }
    }

    return;
}

control_panel::output::Histogram::~Histogram()
{
    delete ui;

    return;
}
