/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/propagate.h"
#include "capture/capture.h"
#include "ui_input_resolution_dialog.h"

InputResolutionDialog::InputResolutionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InputResolutionDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Input resolution");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "input_resolution", this->size()).toSize());
    }

    return;
}

InputResolutionDialog::~InputResolutionDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "input_resolution", this->size());
    }

    delete ui;

    return;
}

// Gets called when a button for forcing the input resolution is pressed in the GUI.
// Will then decide which input resolution to force based on which button was pressed.
void InputResolutionDialog::parse_capture_resolution_button_press(QWidget *button)
{
    QStringList sl;
    resolution_s res = {0, 0, 0};

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

void InputResolutionDialog::activate_capture_res_button(const uint buttonIdx)
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
