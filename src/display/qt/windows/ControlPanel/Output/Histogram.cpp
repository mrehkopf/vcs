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

    ev_new_output_image.listen([this](const image_s &image)
    {
        if (this->isVisible())
        {
            this->ui->histogram->refresh(image);
        }
    });

    ev_capture_signal_lost.listen([this]
    {
        if (this->isVisible())
        {
            this->ui->histogram->clear();
        }
    });

    this->ui->histogram->refresh(ks_scaler_frame_buffer());

    return;
}

control_panel::output::Histogram::~Histogram()
{
    delete ui;

    return;
}
