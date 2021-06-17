/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS resolution dialog
 *
 * A simple dialog that queries the user for a resolution and returns it.
 *
 */

#include "display/qt/dialogs/resolution_dialog.h"
#include "capture/capture_device.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "ui_resolution_dialog.h"

ResolutionDialog::ResolutionDialog(const QString title, resolution_s *const r, QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::ResolutionDialog),
    resolution(r)
{
    k_assert((resolution != nullptr),
             "Expected a valid resolution pointer as an argument to the resolution dialog, but got null.");

    ui->setupUi(this);

    this->set_name(title);

    ui->groupBox->setTitle(title);

    // Set the GUI controls to their proper initial values.
    {
        const resolution_s minres = kc_capture_device().get_minimum_resolution();
        const resolution_s maxres = kc_capture_device().get_maximum_resolution();

        ui->spinBox_x->setMinimum(minres.w);
        ui->spinBox_x->setMaximum(maxres.w);
        ui->spinBox_y->setMinimum(minres.h);
        ui->spinBox_y->setMaximum(maxres.h);
        ui->spinBox_x->setValue(resolution->w);
        ui->spinBox_y->setValue(resolution->h);

        //ui->label_minRes->setText(QString("Minimum: %1 x %2").arg(minres.w).arg(minres.h));
        //ui->label_maxRes->setText(QString("Maximum: %1 x %2").arg(maxres.w).arg(maxres.h));
    }

    // Connect GUI controls to consequences for operating them.
    {
        // Update the target resolution, and exit the dialog.
        connect(ui->pushButton_ok, &QPushButton::clicked, this, [this]
        {
            resolution->w = ui->spinBox_x->value();
            resolution->h = ui->spinBox_y->value();

            this->accept();
        });

        // Exit the dialog without modifying the target resolution.
        connect(ui->pushButton_cancel, &QPushButton::clicked, this, [this]
        {
            this->reject();
        });
    }

    return;
}

ResolutionDialog::~ResolutionDialog(void)
{
    delete ui;
    ui = nullptr;

    return;
}
