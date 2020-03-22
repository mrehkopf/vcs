/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "ui_about_dialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - About");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->groupBox_aboutVCS->setTitle("VCS " + QString(PROGRAM_VERSION_STRING));

    if (DEV_VERSION)
    {
        ui->groupBox_aboutVCS->setTitle(ui->groupBox_aboutVCS->title() + "-dev");
    }

    // We'll set this label visible only when there's a new version of VCS available.
    ui->label_newVersionNotice->setVisible(false);

    // Poll the capture hardware to fill the information matrix about the
    // hardware's capabilities.
    {
        ui->groupBox_captureDeviceInfo->setTitle("Your capture device: " + QString::fromStdString(kc_api().get_device_name()));

        const resolution_s &minres = kc_api().get_minimum_resolution();
        const resolution_s &maxres = kc_api().get_maximum_resolution();

        ui->label_inputMinResolutionString->setText(QString("%1 x %2").arg(minres.w).arg(minres.h));
        ui->label_inputMaxResolutionString->setText(QString("%1 x %2").arg(maxres.w).arg(maxres.h));
        ui->label_inputChannelsString->setText(QString::number(kc_api().get_maximum_input_count()));

        ui->label_firmwareString->setText(QString::fromStdString(kc_api().get_device_firmware_version()));
        ui->label_driverString->setText(QString::fromStdString(kc_api().get_device_driver_version()));

        ui->label_supportsDirectDMAString->setText(kc_api().device_supports_dma()? "Yes" : "No");
        ui->label_supportsDeinterlaceString->setText(kc_api().device_supports_deinterlacing()? "Yes" : "No");
        ui->label_supportsYUVString->setText(kc_api().device_supports_yuv()? "Yes" : "No");
        ui->label_supportsVGACaptureString->setText(kc_api().device_supports_vga()? "Yes" : "No");
        ui->label_supportsDVICaptureString->setText(kc_api().device_supports_dvi()? "Yes" : "No");
        ui->label_supportsCompositeCaptureString->setText(kc_api().device_supports_composite_capture()? "Yes" : "No");
        ui->label_supportsComponentCaptureString->setText(kc_api().device_supports_component_capture()? "Yes" : "No");
        ui->label_supportsSVideoCaptureString->setText(kc_api().device_supports_svideo()? "Yes" : "No");
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "about", this->size()).toSize());
    }

    return;
}

AboutDialog::~AboutDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "about", this->size());
    }

    delete ui;

    return;
}

void AboutDialog::notify_of_new_program_version(void)
{
    ui->label_newVersionNotice->setVisible(true);

    return;
}
