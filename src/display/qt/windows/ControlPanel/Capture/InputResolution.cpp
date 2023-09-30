/*
 * 2019-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "display/qt/widgets/ResolutionQuery.h"
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
        // Wire up buttons for forcing the input resolution.
        for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
        {
            QPushButton *const button = qobject_cast<QPushButton*>(ui->frame_inputForceButtons->layout()->itemAt(i)->widget());

            if (!button)
            {
                continue;
            }

            // Ignore non-resolution-setting buttons.
            if (!button->property("resolution").isValid())
            {
                continue;
            }

            button->setProperty(
                "resolution",
                kpers_value_of(INI_GROUP_CAPTURE, QString("ForceResolution%1").arg(i+1), button->property("resolution")).toSize()
            );

            connect(button, &QPushButton::clicked, this, [this, button, i]
            {
                // If the Alt key is pressed while clicking the button, let the user specify
                // a new resolution for the button.
                if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
                {
                    resolution_s customResolution = {640, 480};

                    if (ResolutionQuery("Assign a custom resolution", &customResolution, parentWidget()).exec() == QDialog::Accepted)
                    {
                        const QSize asQSize = QSize(customResolution.w, customResolution.h);
                        button->setProperty("resolution", asQSize);
                        this->update_button_label(button);
                        kpers_set_value(INI_GROUP_CAPTURE, QString("ForceResolution%1").arg(i+1), asQSize);
                    }
                }
                else
                {
                    const QSize buttonResolution = button->property("resolution").toSize();
                    k_assert(buttonResolution.isValid(), "Detected a malformed input resolution button.");

                    kc_set_device_property("width", buttonResolution.width());
                    kc_set_device_property("height", buttonResolution.height());
                }
            });

            this->update_button_label(button);
        }

        connect(ui->pushButton_applyCustomResolution, &QPushButton::clicked, this, [this]
        {
            kc_set_device_property("width", ui->spinBox_customWidth->value());
            kc_set_device_property("height", ui->spinBox_customHeight->value());
        });
    }

    ev_capture_signal_gained.listen([this]
    {
        this->ui->frame_inputForceButtons->setEnabled(true);
        this->ui->pushButton_applyCustomResolution->setEnabled(true);
        this->ui->spinBox_customWidth->setEnabled(true);
        this->ui->spinBox_customHeight->setEnabled(true);
    });

    ev_capture_signal_lost.listen([this]
    {
        this->ui->frame_inputForceButtons->setEnabled(false);
        this->ui->pushButton_applyCustomResolution->setEnabled(false);
        this->ui->spinBox_customWidth->setEnabled(false);
        this->ui->spinBox_customHeight->setEnabled(false);
    });

    return;
}

control_panel::capture::InputResolution::~InputResolution(void)
{
    delete ui;

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

void control_panel::capture::InputResolution::update_button_label(QPushButton *const button)
{
    const QSize buttonResolution = button->property("resolution").toSize();
    k_assert(buttonResolution.isValid(), "Detected a malformed input resolution button.");

    button->setText(QString("%1 \u00d7 %2").arg(buttonResolution.width()).arg(buttonResolution.height()));

    return;
}
