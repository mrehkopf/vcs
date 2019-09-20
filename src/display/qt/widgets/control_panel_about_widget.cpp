/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * The widget that makes up the control panel's 'About' tab.
 *
 */

#include "display/qt/widgets/control_panel_about_widget.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "ui_control_panel_about_widget.h"

ControlPanelAboutWidget::ControlPanelAboutWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelAboutWidget)
{
    ui->setupUi(this);

    ui->groupBox_aboutVCS->setTitle("VCS " + QString(PROGRAM_VERSION_STRING));

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

    // Connect the GUI controls to consequences for changing their values.
    {
        // Emitted when the user requests the app to use a different GUI styling.
        connect(ui->comboBox_customInterfaceStyling,
                static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),
                this, [this](const QString &styleName)
        {
            /// TODO: There would ideally be a non-hardcoded way to handle styles; and to
            /// allow the user to add their own styles without having to recompile the app.

            QString styleFileName;

            if (styleName == "Disabled")
            {
                // Remove all custom styling.
                styleFileName = "";
            }
            else if (styleName == "Grayscale")
            {
                styleFileName = ":/res/stylesheets/appstyle-gray.qss";
            }
            else
            {
                k_assert(0, "Unknown custom interface style name.");
            }

            emit new_programwide_style_file(styleFileName);
        });
    }

    return;
}

ControlPanelAboutWidget::~ControlPanelAboutWidget()
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

void ControlPanelAboutWidget::restore_persistent_settings(void)
{
    // Custom interface styling is disabled for now.
    #if 0
        set_qcombobox_idx_c(ui->comboBox_customInterfaceStyling)
                            .by_string(kpers_value_of(INI_GROUP_APP, "custom_styling", "Disabled").toString());
    #endif

    return;
}

// Returns true if the user has requested via this dialog to apply a custom,
// program wide styling.
bool ControlPanelAboutWidget::is_custom_program_styling_enabled(void)
{
    return (ui->comboBox_customInterfaceStyling->currentText().toLower() != "disabled");
}
