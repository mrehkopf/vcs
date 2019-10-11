/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/widgets/control_panel_about_widget.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
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
        ui->groupBox_captureDeviceInfo->setTitle("Your capture device: " + QString::fromStdString(kc_hardware().meta.model_name()));

        const resolution_s &minres = kc_hardware().meta.minimum_capture_resolution();
        const resolution_s &maxres = kc_hardware().meta.maximum_capture_resolution();

        ui->label_inputMinResolutionString->setText(QString("%1 x %2").arg(minres.w).arg(minres.h));
        ui->label_inputMaxResolutionString->setText(QString("%1 x %2").arg(maxres.w).arg(maxres.h));
        ui->label_inputChannelsString->setText(QString::number(kc_hardware().meta.num_capture_inputs()));

        ui->label_firmwareString->setText(QString::fromStdString(kc_hardware().meta.firmware_version()));
        ui->label_driverString->setText(QString::fromStdString(kc_hardware().meta.driver_version()));

        ui->label_supportsDirectDMAString->setText(kc_hardware().supports.dma()? "Yes" : "No");
        ui->label_supportsDeinterlaceString->setText(kc_hardware().supports.deinterlace()? "Yes" : "No");
        ui->label_supportsYUVString->setText(kc_hardware().supports.yuv()? "Yes" : "No");
        ui->label_supportsVGACaptureString->setText(kc_hardware().supports.vga()? "Yes" : "No");
        ui->label_supportsDVICaptureString->setText(kc_hardware().supports.dvi()? "Yes" : "No");
        ui->label_supportsCompositeCaptureString->setText(kc_hardware().supports.composite_capture()? "Yes" : "No");
        ui->label_supportsComponentCaptureString->setText(kc_hardware().supports.component_capture()? "Yes" : "No");
        ui->label_supportsSVideoCaptureString->setText(kc_hardware().supports.svideo()? "Yes" : "No");
    }

    return;
}

AboutDialog::~AboutDialog()
{
    // Save persistent settings.
    {
        // Custom interface styling is disabled for now.
        #if 0
            kpers_set_value(INI_GROUP_APP, "custom_styling", ui->comboBox_customInterfaceStyling->currentText());
        #endif
    }

    delete ui;

    return;
}

void AboutDialog::notify_of_new_program_version(void)
{
    ui->label_newVersionNotice->setVisible(true);

    return;
}
