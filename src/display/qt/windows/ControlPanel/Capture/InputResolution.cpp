/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/persistent_settings.h"
#include "common/vcs_event/vcs_event.h"
#include "capture/capture.h"
#include "InputResolution.h"
#include "ui_InputResolution.h"

control_panel::capture::InputResolution::InputResolution(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::InputResolution)
{
    ui->setupUi(this);

    this->set_name("Input resolution");

    // Set the GUI controls to their proper initial values.
    {
        ui->spinBox_customWidth->setMaximum(MAX_CAPTURE_WIDTH);
        ui->spinBox_customWidth->setMinimum(MIN_CAPTURE_WIDTH);
        ui->spinBox_customHeight->setMaximum(MAX_CAPTURE_HEIGHT);
        ui->spinBox_customHeight->setMinimum(MIN_CAPTURE_HEIGHT);
    }

    // Connect GUI controls to consequences for operating them.
    {
        // Wire up buttons for forcing the capture input resolution.
        for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
        {
            QPushButton *const button = qobject_cast<QPushButton*>(ui->frame_inputForceButtons->layout()->itemAt(i)->widget());

            if (!button)
            {
                continue;
            }

            // Sanity check. Buttons that force the resolution have no label text but
            // include the "resolution" dynamic property. Auxiliary buttons don't define
            // the property but have a label text. Other buttons are considered malformed.
            k_assert(
                !button->text().isEmpty() ||
                button->property("resolution").isValid(),
                "Detected a malformed input resolution button."
            );

            // Ignore non-resolution-setting buttons.
            if (!button->property("resolution").isValid())
            {
                continue;
            }

            this->update_button_text(button);

            connect(button, &QPushButton::clicked, this, [this, button]
            {
                const QSize buttonResolution = button->property("resolution").toSize();
                k_assert(buttonResolution.isValid(), "Detected a malformed input resolution button.");

                kc_set_device_property("width", buttonResolution.width());
                kc_set_device_property("height", buttonResolution.height());
            });
        }

        connect(ui->pushButton_applyCustomResolution, &QPushButton::clicked, this, [this]
        {
            kc_set_device_property("width", ui->spinBox_customWidth->value());
            kc_set_device_property("height", ui->spinBox_customHeight->value());
        });
    }

    ev_capture_signal_gained.listen([this]{this->ui->frame_inputForceButtons->setEnabled(true);});
    ev_capture_signal_lost.listen([this]{this->ui->frame_inputForceButtons->setEnabled(false);});

    return;
}

control_panel::capture::InputResolution::~InputResolution(void)
{
    delete ui;

    return;
}

void control_panel::capture::InputResolution::update_button_text(QPushButton *const button)
{
    const QSize buttonResolution = button->property("resolution").toSize();

    if (buttonResolution.isValid())
    {
        button->setText(QString("%1 \u00d7 %2").arg(buttonResolution.width()). arg(buttonResolution.height()));
    }

    return;
}

void control_panel::capture::InputResolution::activate_resolution_button(const uint buttonIdx)
{
    for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
    {
        QPushButton *const button = qobject_cast<QPushButton*>(ui->frame_inputForceButtons->layout()->itemAt(i)->widget());

        if (!button)
        {
            continue;
        }

        if (button->property("button_idx").toUInt() == buttonIdx)
        {
            button->click();
            return;
        }
    }

    NBENE(("Failed to find an input resolution button at index #%u.", buttonIdx));

    return;
}
