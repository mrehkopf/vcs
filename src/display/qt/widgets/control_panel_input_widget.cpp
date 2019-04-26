/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * The widget that makes up the control panel's 'Input' tab.
 *
 */

#include "display/qt/widgets/control_panel_input_widget.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/propagate.h"
#include "capture/capture.h"
#include "ui_control_panel_input_widget.h"

ControlPanelInputWidget::ControlPanelInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelInputWidget)
{
    ui->setupUi(this);

    // Initialize the GUI controls.
    {
        reset_color_depth_combobox_selection();

        // Populate the list of the capture hardware's input channels.
        {
            block_widget_signals_c b(ui->comboBox_inputChannel);

            ui->comboBox_inputChannel->clear();

            for (int i = 0; i < kc_hardware().meta.num_capture_inputs(); i++)
            {
                ui->comboBox_inputChannel->addItem(QString("Channel #%1").arg((i + 1)));
            }

            ui->comboBox_inputChannel->setCurrentIndex(INPUT_CHANNEL_IDX);

            // Lock the input channel selector if only one channel is available.
            if (ui->comboBox_inputChannel->count() == 1)
            {
                ui->comboBox_inputChannel->setEnabled(false);
            }
        }
    }

    // Wire up the buttons for forcing the capture input resolution.
    {
        for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
        {
            QWidget *const w = ui->frame_inputForceButtons->layout()->itemAt(i)->widget();

            k_assert(w->objectName().contains("pushButton"), "Expected all widgets in this layout to be pushbuttons.");

            // Store the unique id of this button, so we can later identify it.
            ((QPushButton*)w)->setProperty("butt_id", i);

            // Load in any custom resolutions the user may have set earlier.
            if (kpers_contains(INI_GROUP_INPUT, QString("force_res_%1").arg(i)))
            {
                ((QPushButton*)w)->setText(kpers_value_of(INI_GROUP_INPUT, QString("force_res_%1").arg(i)).toString());
            }

            connect((QPushButton*)w, &QPushButton::clicked,
                               this, [this,w]{this->parse_capture_resolution_button_press(w);});
        }
    }

    return;
}

ControlPanelInputWidget::~ControlPanelInputWidget()
{
    delete ui;

    return;
}

void ControlPanelInputWidget::restore_persistent_settings(void)
{
    /// No persistent settings for this dialog, yet.

    return;
}

void ControlPanelInputWidget::set_capture_info_as_no_signal(void)
{
    ui->label_captInputResolution->setText("n/a");

    if (kc_is_invalid_signal())
    {
        ui->label_captInputSignal->setText("Invalid signal");
    }
    else
    {
        ui->label_captInputSignal->setText("No signal");
    }

    set_input_controls_enabled(false);

    return;
}

void ControlPanelInputWidget::set_capture_info_as_receiving_signal(void)
{
    set_input_controls_enabled(true);

    return;
}

void ControlPanelInputWidget::update_capture_signal_info(void)
{
    const capture_signal_s s = kc_hardware().status.signal();

    // Mark the resolution. If either dimenson is 0, we expect it to be an
    // invalid reading that should be ignored.
    if (s.r.w == 0 || s.r.h == 0)
    {
        ui->label_captInputResolution->setText("n/a");
    }
    else
    {
        QString res = QString("%1 x %2").arg(s.r.w).arg(s.r.h);

        ui->label_captInputResolution->setText(res);
    }

    // Mark the refresh rate. If the value is 0, we expect it to be an invalid
    // reading and that we should ignore it.
    if (s.refreshRate != 0)
    {
        const QString t = ui->label_captInputResolution->text();

        ui->label_captInputResolution->setText(t + QString(", %1 Hz").arg(s.refreshRate));
    }

    ui->label_captInputSignal->setText(s.isDigital? "Digital" : "Analog");

    return;
}

void ControlPanelInputWidget::set_input_controls_enabled(const bool state)
{
    ui->frame_inputForceButtons->setEnabled(state);
    ui->pushButton_inputAdjustVideoColor->setEnabled(state);
    ui->comboBox_frameSkip->setEnabled(state);
    ui->comboBox_bitDepth->setEnabled(state);
    ui->label_captInputResolution->setEnabled(state);

    return;
}

void ControlPanelInputWidget::activate_capture_res_button(const uint buttonIdx)
{
    for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
    {
        QWidget *const w = ui->frame_inputForceButtons->layout()->itemAt(i)->widget();

        /// A bit kludgy, but...
        if (w->objectName().endsWith(QString::number(buttonIdx)))
        {
            parse_capture_resolution_button_press(w);
            return;
        }
    }

    NBENE(("Failed to find input resolution button #%u.", buttonIdx));

    return;
}

// Gets called when a button for forcing the input resolution is pressed in the GUI.
// Will then decide which input resolution to force based on which button was pressed.
void ControlPanelInputWidget::parse_capture_resolution_button_press(QWidget *button)
{
    QStringList sl;
    resolution_s res = {0, 0};

    k_assert(button->objectName().contains("pushButton"),
             "Expected a button widget, but received something else.");

    // Query the user for a custom resolution.
    /// TODO. Get a more reliable way.
    if (((QPushButton*)button)->text() == "Other...")
    {
        res.w = 1920;
        res.h = 1080;
        if (ResolutionDialog("Force an input resolution",
                             &res, parentWidget()).exec() == QDialog::Rejected)
        {
            // If the user canceled.
            goto done;
        }

        goto assign_resolution;
    }

    // Extract the resolution from the button name. The name is expected to be
    // of the form e.g. '640 x 480' or '640x480'.
    sl = ((QPushButton*)button)->text().split('x');
    if (sl.size() < 2)
    {
        DEBUG(("Unexpected number of parameters in a button name. Expected at least width and height."));
        goto done;
    }
    else
    {
        res.w = sl.at(0).toUInt();
        res.h = sl.at(1).toUInt();
    }

    // If alt is pressed while clicking the button, let the user specify a new
    // resolution for the button.
    if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
    {
        // Pop up a dialog asking the user for the new resolution.
        if (ResolutionDialog("Assign an input resolution",
                             &res, parentWidget()).exec() != QDialog::Rejected)
        {
            const QString resolutionStr = QString("%1 x %2").arg(res.w).arg(res.h);

            ((QPushButton*)button)->setText(resolutionStr);

            // Save the new resolution into the ini.
            kpers_set_value(INI_GROUP_INPUT,
                            QString("force_res_%1").arg(((QPushButton*)button)->property("butt_id").toUInt()),
                            resolutionStr);

            DEBUG(("Assigned a new resolution (%u x %u) for an input force button.",
                   res.w, res.h));
        }

        goto done;
    }

    assign_resolution:
    DEBUG(("Received a request via the GUI to set the input resolution to %u x %u.", res.w, res.h));
    kpropagate_forced_capture_resolution(res);

    done:
    return;
}

// Queries the current capture input bit depth and sets the GUI combobox selection
// accordingly.
//
void ControlPanelInputWidget::reset_color_depth_combobox_selection()
{
    QString depthString = QString("%1-bit").arg(kc_input_color_depth()); // E.g. "24-bit".

    for (int i = 0; i < ui->comboBox_bitDepth->count(); i++)
    {
        if (ui->comboBox_bitDepth->itemText(i).contains(depthString))
        {
            ui->comboBox_bitDepth->setCurrentIndex(i);
            goto done;
        }
    }

    k_assert(0, "Failed to set up the GUI for the current capture bit depth.");

    done:
    return;
}

void ControlPanelInputWidget::on_comboBox_bitDepth_currentIndexChanged(const QString &bitDepthString)
{
    const uint bpp = ([bitDepthString]()->uint
    {
        if (bitDepthString.contains("24-bit")) return 24;
        else if (bitDepthString.contains("16-bit")) return 16;
        else if (bitDepthString.contains("15-bit")) return 15;
        else
        {
            k_assert(0, "Unrecognized color depth option in the GUI dropbox.");
            return 0;
        }
    })();

    if (!kc_set_input_color_depth(bpp))
    {
        reset_color_depth_combobox_selection();

        kd_show_headless_error_message("", "Failed to change the capture color depth.\n\n"
                                           "The previous setting has been restored.");
    }

    return;
}

void ControlPanelInputWidget::on_comboBox_inputChannel_currentIndexChanged(int index)
{
    // If we fail to set the input channel, revert back to the current one.
    if (!kc_set_input_channel(index))
    {
        block_widget_signals_c b(ui->comboBox_inputChannel);

        NBENE(("Failed to set the input channel to %d. Reverting.", index));
        ui->comboBox_inputChannel->setCurrentIndex(kc_input_channel_idx());
    }

    return;
}

void ControlPanelInputWidget::on_comboBox_frameSkip_currentIndexChanged(const QString &skipString)
{
    const uint skipNum = ([skipString]()->uint
    {
        if (skipString == "None") return 0;
        else if (skipString == "Half") return 1;
        else if (skipString == "Two thirds") return 2;
        else if (skipString == "Three quarters") return 3;
        else
        {
            k_assert(0, "Unexpected GUI string for frame-skipping.");
            return 0;
        }
    })();

    kc_set_frame_dropping(skipNum);

    return;
}

void ControlPanelInputWidget::on_pushButton_inputAdjustVideoColor_clicked(void)
{
    emit open_video_adjust_dialog();

    return;
}

void ControlPanelInputWidget::on_pushButton_inputAliases_clicked(void)
{
    emit open_alias_dialog();

    return;
}
