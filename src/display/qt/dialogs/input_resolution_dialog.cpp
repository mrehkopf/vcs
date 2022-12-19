/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "ui_input_resolution_dialog.h"

InputResolutionDialog::InputResolutionDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::InputResolutionDialog)
{
    ui->setupUi(this);

    this->set_name("Input resolution");

    ui->label_warnOfDigitalInput->setVisible(false);

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
            k_assert((!button->text().isEmpty() ||
                      button->property("resolution").isValid()),
                     "Detected a malformed input resolution button.");

            // Ignore non-resolution-setting buttons.
            if (!button->property("resolution").isValid())
            {
                continue;
            }

            // Load in any custom resolution the user may have set for this button.
            if (kpers_contains(INI_GROUP_INPUT, QString("force_res_%1").arg(i)))
            {
                const QSize customResolution = kpers_value_of(INI_GROUP_INPUT, QString("force_res_%1").arg(i)).toSize();

                if (customResolution.isValid())
                {
                    button->setProperty("resolution", customResolution);
                }
            }

            this->update_button_text(button);

            connect(button, &QPushButton::clicked, this, [this, button]
            {
                const QSize buttonResolution = button->property("resolution").toSize();
                k_assert(buttonResolution.isValid(), "Detected a malformed input resolution button.");

                // If the Alt key is pressed while clicking the button, let the user specify
                // a new resolution for the button.
                if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
                {
                    resolution_s customResolution = {unsigned(buttonResolution.width()),
                                                     unsigned(buttonResolution.height())};

                    if (ResolutionDialog("Assign an input resolution", &customResolution, parentWidget()).exec() != QDialog::Rejected)
                    {
                        button->setProperty("resolution", QSize(customResolution.w, customResolution.h));

                        this->update_button_text(button);

                        kpers_set_value(INI_GROUP_INPUT,
                                        QString("force_res_%1").arg(button->property("button_idx").toUInt()),
                                        button->property("resolution"));
                    }
                }
                else
                {
                    kc_force_capture_resolution({unsigned(buttonResolution.width()),
                                               unsigned(buttonResolution.height())});
                }
            });
        }

        connect(ui->pushButton_setCustomResolution, &QPushButton::clicked, this, [this]
        {
            resolution_s customResolution = {1920, 1080};

            if (ResolutionDialog("Force a capture resolution", &customResolution, parentWidget()).exec() == QDialog::Accepted)
            {
                kc_force_capture_resolution(customResolution);
            }
        });
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "input_resolution", this->size()).toSize());
    }

    // Register app event listeners.
    {
        kc_evNewVideoMode.listen([this](const video_mode_s&)
        {
            this->ui->label_warnOfDigitalInput->setVisible(kc_current_capture_state().signalFormat == signal_format_e::digital);
        });

        kc_evSignalLost.listen([this]
        {
            this->ui->label_warnOfDigitalInput->setVisible(false);
        });
    }

    return;
}

InputResolutionDialog::~InputResolutionDialog(void)
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "input_resolution", this->size());
    }

    delete ui;

    return;
}

void InputResolutionDialog::update_button_text(QPushButton *const button)
{
    const QSize buttonResolution = button->property("resolution").toSize();

    if (buttonResolution.isValid())
    {
        button->setText(QString("%1 \u00d7 %2").arg(buttonResolution.width()). arg(buttonResolution.height()));
    }

    return;
}

void InputResolutionDialog::activate_resolution_button(const uint buttonIdx)
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
