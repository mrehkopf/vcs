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

    // Fill the feature matrix of the capture device's capabilities.
    {
        const auto add_feature = [this](const QString featureName, const QString featureValue)
        {
            const int rowIdx = ui->tableWidget_captureDeviceFeatures->rowCount();

            ui->tableWidget_captureDeviceFeatures->insertRow(ui->tableWidget_captureDeviceFeatures->rowCount());

            ui->tableWidget_captureDeviceFeatures->setItem(rowIdx, 0, new QTableWidgetItem(featureName));
            ui->tableWidget_captureDeviceFeatures->setItem(rowIdx, 1, new QTableWidgetItem(featureValue));
        };

        ui->groupBox_captureDeviceInfo->setTitle("Capture device: " + QString::fromStdString(kc_capture_api().get_device_name()));

        ui->tableWidget_captureDeviceFeatures->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->tableWidget_captureDeviceFeatures->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_captureDeviceFeatures->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

        const resolution_s &minres = kc_capture_api().get_minimum_resolution();
        const resolution_s &maxres = kc_capture_api().get_maximum_resolution();

        add_feature("Minimum resolution", QString("%1 x %2").arg(minres.w).arg(minres.h));
        add_feature("Maximum resolution", QString("%1 x %2").arg(maxres.w).arg(maxres.h));
        add_feature("Firmware version",   QString::fromStdString(kc_capture_api().get_device_firmware_version()));
        add_feature("Driver version",     QString::fromStdString(kc_capture_api().get_device_driver_version()));
        add_feature("Capture API",        QString::fromStdString(kc_capture_api().get_api_name()));
        add_feature("DMA transfer",       (kc_capture_api().device_supports_dma()?               "Supported" : "Not supported"));
        add_feature("Deinterlacing",      (kc_capture_api().device_supports_deinterlacing()?     "Supported" : "Not supported"));
        add_feature("YUV encoding",       (kc_capture_api().device_supports_yuv()?               "Supported" : "Not supported"));
        add_feature("VGA capture",        (kc_capture_api().device_supports_vga()?               "Supported" : "Not supported"));
        add_feature("DVI capture",        (kc_capture_api().device_supports_dvi()?               "Supported" : "Not supported"));
        add_feature("Component capture",  (kc_capture_api().device_supports_component_capture()? "Supported" : "Not supported"));
        add_feature("Composite capture",  (kc_capture_api().device_supports_composite_capture()? "Supported" : "Not supported"));
        add_feature("S-Video capture",    (kc_capture_api().device_supports_svideo()?            "Supported" : "Not supported"));
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
