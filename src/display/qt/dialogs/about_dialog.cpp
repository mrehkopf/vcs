/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
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

        #ifndef VCS_RELEASE_BUILD
            ui->groupBox_aboutVCS->setTitle(ui->groupBox_aboutVCS->title() + " (non-release build)");
        #endif

        ui->label_aboutVCS->setText(ui->label_aboutVCS->text().replace("%VCS_VERSION_STRING%", PROGRAM_VERSION_STRING));
    }

    // Fill the feature matrix of the capture device's capabilities.
    {
        ui->groupBox_captureDeviceInfo->setTitle(QString::fromStdString(kc_get_device_name()));

        const resolution_s &minres = kc_get_device_minimum_resolution();
        const resolution_s &maxres = kc_get_device_maximum_resolution();

        ui->tableWidget_captureDeviceFeatures->modify_property("Minimum input resolution", QString("%1 \u00d7 %2").arg(minres.w).arg(minres.h));
        ui->tableWidget_captureDeviceFeatures->modify_property("Maximum input resolution", QString("%1 \u00d7 %2").arg(maxres.w).arg(maxres.h));
        ui->tableWidget_captureDeviceFeatures->modify_property("Channel count", QString::number(kc_get_device_maximum_input_count()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Firmware", QString::fromStdString(kc_get_device_firmware_version()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Driver", QString::fromStdString(kc_get_device_driver_version()));
        ui->tableWidget_captureDeviceFeatures->modify_property("Capture backend", QString::fromStdString(kc_get_device_api_name()));
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
