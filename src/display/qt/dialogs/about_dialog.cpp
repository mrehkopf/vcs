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
    VCSBaseDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    this->set_name("About");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    {
        ui->groupBox_aboutVCS->setTitle("VCS " + QString(PROGRAM_VERSION_STRING));

        if (IS_DEV_VERSION)
        {
            ui->groupBox_aboutVCS->setTitle(ui->groupBox_aboutVCS->title() + "~dev");
        }
    }

    // We'll set this label visible only when there's a new version of VCS available.
    ui->label_newVersionNotice->setVisible(false);

    // Fill the feature matrix of the capture device's capabilities.
    {
        ui->groupBox_captureDeviceInfo->setTitle("Capture device: " + QString::fromStdString(kc_capture_api().get_device_name()));

        const resolution_s &minres = kc_capture_api().get_minimum_resolution();
        const resolution_s &maxres = kc_capture_api().get_maximum_resolution();

        ui->tableWidget_captureDeviceFeatures->modify_property("Minimum resolution", QString("%1 x %2").arg(minres.w).arg(minres.h));
        ui->tableWidget_captureDeviceFeatures->modify_property("Maximum resolution", QString("%1 x %2").arg(maxres.w).arg(maxres.h));
        ui->tableWidget_captureDeviceFeatures->modify_property("Input channels",     QString::number(kc_capture_api().get_device_maximum_input_count()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Firmware",           QString::fromStdString(kc_capture_api().get_device_firmware_version()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Driver",             QString::fromStdString(kc_capture_api().get_device_driver_version()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Capture API",        QString::fromStdString(kc_capture_api().get_api_name()));
        #ifndef __linux__ // The Linux capture API doesn't support querying these parameters.
            ui->tableWidget_captureDeviceFeatures->modify_property("DMA transfer",       (kc_capture_api().device_supports_dma()?               "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("Deinterlacing",      (kc_capture_api().device_supports_deinterlacing()?     "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("YUV encoding",       (kc_capture_api().device_supports_yuv()?               "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("VGA capture",        (kc_capture_api().device_supports_vga()?               "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("DVI capture",        (kc_capture_api().device_supports_dvi()?               "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("Component capture",  (kc_capture_api().device_supports_component_capture()? "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("Composite capture",  (kc_capture_api().device_supports_composite_capture()? "Supported" : "Not supported"));
            ui->tableWidget_captureDeviceFeatures->modify_property("S-Video capture",    (kc_capture_api().device_supports_svideo()?            "Supported" : "Not supported"));
        #endif
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
