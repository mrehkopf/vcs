/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS video & color dialog
 *
 * A GUI dialog for allowing the user to adjust the capture card's video and color
 * parameters.
 *
 */

#include <QElapsedTimer>
#include <QFileDialog>
#include <QPushButton>
#include <QFileInfo>
#include <QToolBar>
#include <QMenuBar>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include "display/qt/subclasses/QTableWidget_property_table.h"
#include "display/qt/dialogs/signal_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/video_parameters.h"
#include "capture/capture.h"
#include "common/disk/disk.h"
#include "ui_signal_dialog.h"

/*
 * TODOS:
 *
 * - the dialog should communicate more clearly which mode the current GUI controls
 *   are editing, and which modes are available or known about.
 *
 */

// By default, values from the GUI's controls (sliders and spinboxes) will be
// sent to the capture card in real-time, i.e. as the controls are operated.
// Set this variable to false to disable real-time updates.
static bool CONTROLS_LIVE_UPDATE = true;

static const QString BASE_WINDOW_TITLE = "VCS - Signal";

// Used to keep track of how long we've had a particular video mode set.
static QElapsedTimer VIDEO_MODE_UPTIME;
static QTimer VIDEO_MODE_UPTIME_UPDATE;

SignalDialog::SignalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SignalDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(BASE_WINDOW_TITLE);

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information.
        {
            ui->tableWidget_propertyTable->modify_property("Uptime", "00:00:00");
            ui->tableWidget_propertyTable->modify_property("Resolution", "");
            ui->tableWidget_propertyTable->modify_property("Refresh rate", "");
        }

        // Start timers to keep track of the video mode's uptime.
        {
            VIDEO_MODE_UPTIME.start();
            VIDEO_MODE_UPTIME_UPDATE.start(1000);

            connect(&VIDEO_MODE_UPTIME_UPDATE, &QTimer::timeout, [this]
            {
                const unsigned seconds = unsigned(VIDEO_MODE_UPTIME.elapsed() / 1000);
                const unsigned minutes = (seconds / 60);
                const unsigned hours   = (minutes / 60);

                ui->tableWidget_propertyTable->modify_property("Uptime", QString("%1:%2:%3").arg(QString::number(hours).rightJustified(2, '0'))
                                                                                            .arg(QString::number(minutes % 60).rightJustified(2, '0'))
                                                                                            .arg(QString::number(seconds % 60).rightJustified(2, '0')));
            });
        }

        // Create the dialog's menu bar.
        {
            this->menubar = new QMenuBar(this);

            // File...
            {
                QMenu *fileMenu = new QMenu("File", this);

                fileMenu->addAction("Load parameters...", this, SLOT(load_settings()));
                fileMenu->addSeparator();
                fileMenu->addAction("Save parameters as...", this, SLOT(save_settings()));

                menubar->addMenu(fileMenu);
            }

            this->layout()->setMenuBar(menubar);
        }

        // Connect GUI controls to consequences for operating them.
        {
            // Loops through each spinbox to find the corresponding slider to connect their
            // valueChanged signals.
            auto connect_spinboxes_to_their_sliders = [=](QGroupBox *const parent)
            {
                for (int i = 0; i < parent->layout()->count(); i++)
                {
                    QWidget *const spinbox = parent->layout()->itemAt(i)->widget();

                    if (!spinbox || !spinbox->objectName().startsWith("spinBox_"))
                    {
                        continue;
                    }

                    // Expect spinboxes in the GUI to be named 'spinbox_blabla', and for the
                    // corresponding sliders to be called 'slider_blabla', where 'slider' is
                    // e.g. 'horizontalScrollBar' and 'blabla' is a string identical to that
                    // of the slider's.
                    QStringList nameParts = spinbox->objectName().split("_");
                    k_assert((nameParts.size() == 2), "Expected a spinbox name in the form xxxx_name.");
                    QString spinboxName = nameParts.at(1);

                    // Loop through to find the slider with a matching name.
                    for (int i = 0; i < parent->layout()->count(); i++)
                    {
                        QWidget *const slider = parent->layout()->itemAt(i)->widget();

                        if (slider != nullptr &&
                            slider->objectName().startsWith("horizontalScrollBar_") &&
                            slider->objectName().split("_").at(1) == spinboxName &&
                            slider->objectName().endsWith(spinboxName))
                        {
                            // Found it, so hook the two up.

                            // The valueChanged() signal is overloaded for int and QString, and we
                            // have to choose one. I'm using Qt 5.5, but you may have better ways
                            // of doing this in later versions.
                            #define OVERLOAD_INT static_cast<void (QSpinBox::*)(int)>

                            connect((QSpinBox*)spinbox, OVERLOAD_INT(&QSpinBox::valueChanged),
                                    (QScrollBar*)slider, &QScrollBar::setValue);

                            connect((QScrollBar*)slider, &QScrollBar::valueChanged,
                                    (QSpinBox*)spinbox, &QSpinBox::setValue);

                            connect((QSpinBox*)spinbox, OVERLOAD_INT(&QSpinBox::valueChanged),
                                    this, &SignalDialog::broadcast_settings);

                            #undef OVERLOAD_INT

                            goto next_spinbox;
                        }
                    }

                    k_assert(0, "Found a spinbox without a matching slider. Make sure each "
                                "slider-spinbox combination shares a name (each spinbox is "
                                "expected to be matched with a slider in this dialog).");

                    next_spinbox:
                    continue;
                }

                return;
            };

            connect_spinboxes_to_their_sliders(ui->groupBox_videoAdjust);

            update_controls();
            set_controls_enabled(true);
        }
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "signal", this->size()).toSize());
    }

    return;
}

SignalDialog::~SignalDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "signal", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

// Called to inform the dialog that a new capture signal has been received.
void SignalDialog::notify_of_new_capture_signal(void)
{
    VIDEO_MODE_UPTIME.restart();
    update_controls();
    update_information_table(true);

    return;
}

// Called to ask the dialog to poll the capture hardware for its current video
// and color settings.
void SignalDialog::update_controls(void)
{
    CONTROLS_LIVE_UPDATE = false;

    // Current values.
    {
        const video_signal_parameters_s p = kc_capture_api().get_video_signal_parameters();

        ui->spinBox_colorBright->setValue(p.overallBrightness);
        ui->horizontalScrollBar_colorBright->setValue(p.overallBrightness);

        ui->spinBox_colorContr->setValue(p.overallContrast);
        ui->horizontalScrollBar_colorContr->setValue(p.overallContrast);

        ui->spinBox_colorBrightRed->setValue(p.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setValue(p.redBrightness);

        ui->spinBox_colorContrRed->setValue(p.redContrast);
        ui->horizontalScrollBar_colorContrRed->setValue(p.redContrast);

        ui->spinBox_colorBrightGreen->setValue(p.greenBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setValue(p.greenBrightness);

        ui->spinBox_colorContrGreen->setValue(p.greenContrast);
        ui->horizontalScrollBar_colorContrGreen->setValue(p.greenContrast);

        ui->spinBox_colorBrightBlue->setValue(p.blueBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setValue(p.blueBrightness);

        ui->spinBox_colorContrBlue->setValue(p.blueContrast);
        ui->horizontalScrollBar_colorContrBlue->setValue(p.blueContrast);

        ui->spinBox_videoBlackLevel->setValue(p.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setValue(p.blackLevel);

        ui->spinBox_videoHorPos->setValue(p.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setValue(p.horizontalPosition);

        ui->spinBox_videoHorSize->setValue(p.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setValue(p.horizontalScale);

        ui->spinBox_videoPhase->setValue(p.phase);
        ui->horizontalScrollBar_videoPhase->setValue(p.phase);

        ui->spinBox_videoVerPos->setValue(p.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setValue(p.verticalPosition);
    }

    // Current valid ranges.
    {
        const video_signal_parameters_s min = kc_capture_api().get_minimum_video_signal_parameters();
        const video_signal_parameters_s max = kc_capture_api().get_maximum_video_signal_parameters();

        ui->spinBox_colorBright->setMinimum(min.overallBrightness);
        ui->spinBox_colorBright->setMaximum(max.overallBrightness);
        ui->horizontalScrollBar_colorBright->setMinimum(min.overallBrightness);
        ui->horizontalScrollBar_colorBright->setMaximum(max.overallBrightness);

        ui->spinBox_colorContr->setMinimum(min.overallContrast);
        ui->spinBox_colorContr->setMaximum(max.overallContrast);
        ui->horizontalScrollBar_colorContr->setMinimum(min.overallContrast);
        ui->horizontalScrollBar_colorContr->setMaximum(max.overallContrast);

        ui->spinBox_colorBrightRed->setMinimum(min.redBrightness);
        ui->spinBox_colorBrightRed->setMaximum(max.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setMinimum(min.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setMaximum(max.redBrightness);

        ui->spinBox_colorContrRed->setMinimum(min.redContrast);
        ui->spinBox_colorContrRed->setMaximum(max.redContrast);
        ui->horizontalScrollBar_colorContrRed->setMinimum(min.redContrast);
        ui->horizontalScrollBar_colorContrRed->setMaximum(max.redContrast);

        ui->spinBox_colorBrightGreen->setMinimum(min.blueBrightness);
        ui->spinBox_colorBrightGreen->setMaximum(max.blueBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setMinimum(min.blueBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setMaximum(max.blueBrightness);

        ui->spinBox_colorContrGreen->setMinimum(min.blueContrast);
        ui->spinBox_colorContrGreen->setMaximum(max.blueContrast);
        ui->horizontalScrollBar_colorContrGreen->setMinimum(min.blueContrast);
        ui->horizontalScrollBar_colorContrGreen->setMaximum(max.blueContrast);

        ui->spinBox_colorBrightBlue->setMinimum(min.greenBrightness);
        ui->spinBox_colorBrightBlue->setMaximum(max.greenBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setMinimum(min.greenBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setMaximum(max.greenBrightness);

        ui->spinBox_colorContrBlue->setMinimum(min.greenContrast);
        ui->spinBox_colorContrBlue->setMaximum(max.greenContrast);
        ui->horizontalScrollBar_colorContrBlue->setMinimum(min.greenContrast);
        ui->horizontalScrollBar_colorContrBlue->setMaximum(max.greenContrast);

        ui->spinBox_videoBlackLevel->setMinimum(min.blackLevel);
        ui->spinBox_videoBlackLevel->setMaximum(max.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMinimum(min.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMaximum(max.blackLevel);

        ui->spinBox_videoHorPos->setMinimum(min.horizontalPosition);
        ui->spinBox_videoHorPos->setMaximum(max.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setMinimum(min.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setMaximum(max.horizontalPosition);

        ui->spinBox_videoHorSize->setMinimum(min.horizontalScale);
        ui->spinBox_videoHorSize->setMaximum(max.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setMinimum(min.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setMaximum(max.horizontalScale);

        ui->spinBox_videoPhase->setMinimum(min.phase);
        ui->spinBox_videoPhase->setMaximum(max.phase);
        ui->horizontalScrollBar_videoPhase->setMinimum(min.phase);
        ui->horizontalScrollBar_videoPhase->setMaximum(max.phase);

        ui->spinBox_videoVerPos->setMinimum(min.verticalPosition);
        ui->spinBox_videoVerPos->setMaximum(max.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setMinimum(min.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setMaximum(max.verticalPosition);
    }

    CONTROLS_LIVE_UPDATE = true;

    flag_unsaved_changes();

    return;
}

void SignalDialog::set_controls_enabled(const bool state)
{
    // Note: At the time of this writing, this function gets called only
    // to notify the dialog that the input signal has been lost (state ==
    // false) or gained (state == true).
    VIDEO_MODE_UPTIME.restart();
    ui->groupBox_videoAdjust->setEnabled(state);
    update_information_table(state);

    return;
}

void SignalDialog::update_information_table(const bool isReceivingSignal)
{
    if (isReceivingSignal)
    {
        const resolution_s resolution = kc_capture_api().get_resolution();
        const unsigned refreshRate = kc_capture_api().get_refresh_rate();

        ui->tableWidget_propertyTable->modify_property("Refresh rate", QString("%1 Hz").arg(QString::number(refreshRate)));
        ui->tableWidget_propertyTable->modify_property("Resolution", QString("%1 x %2").arg(resolution.w)
                                                                                       .arg(resolution.h));
    }
    else
    {
        ui->tableWidget_propertyTable->modify_property("Resolution", "No signal");
        ui->tableWidget_propertyTable->modify_property("Refresh rate", "No signal");
    }

    return;
}

// Tally up all the current video and color settings from the GUI, and ask the
// capture card to adopt them.
void SignalDialog::broadcast_settings(void)
{
    if (!CONTROLS_LIVE_UPDATE) return;

    video_signal_parameters_s p;

    p.overallBrightness = ui->spinBox_colorBright->value();
    p.overallContrast = ui->spinBox_colorContr->value();
    p.redBrightness = ui->spinBox_colorBrightRed->value();
    p.redContrast = ui->spinBox_colorContrRed->value();
    p.greenBrightness = ui->spinBox_colorBrightGreen->value();
    p.greenContrast = ui->spinBox_colorContrGreen->value();
    p.blueBrightness = ui->spinBox_colorBrightBlue->value();
    p.blueContrast = ui->spinBox_colorContrBlue->value();
    p.blackLevel = ui->spinBox_videoBlackLevel->value();
    p.horizontalPosition = ui->spinBox_videoHorPos->value();
    p.horizontalScale = ui->spinBox_videoHorSize->value();
    p.phase = ui->spinBox_videoPhase->value();
    p.verticalPosition = ui->spinBox_videoVerPos->value();

    kc_capture_api().set_video_signal_parameters(p);

    flag_unsaved_changes();

    return;
}

// Activates a user-facing visual indication that there are unsaved changes in
// the dialog.
void SignalDialog::flag_unsaved_changes(void)
{
    /// Disabled for now.
    return;

    if (!this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().prepend("*"));
    }

    return;
}

// Deactivates the user-facing visual indication that there are unsaved changes
// in the dialog.
void SignalDialog::remove_unsaved_changes_flag(void)
{
    if (this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().remove(0, 1));
    }

    return;
}

// Called to inform the dialog of a new source file for video and color parameters.
void SignalDialog::receive_new_mode_settings_filename(const QString &filename)
{
    const QString newWindowTitle = QString("%1%2 - %3").arg(this->windowTitle().startsWith("*")? "*" : "")
                                                       .arg(BASE_WINDOW_TITLE)
                                                       .arg(QFileInfo(filename).baseName());

    this->setWindowTitle(newWindowTitle);

    return;
}

void SignalDialog::save_settings(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save video and color settings as...", "",
                                                    "Mode parameters (*.vcs-video *.vcsm);;All files(*.*)");

    if (filename.isEmpty()) return;

    if (!QStringList({"vcs-video", "vcsm"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-video");
    }

    if (kdisk_save_video_signal_parameters(kvideoparam_all_parameter_sets(), filename))
    {
        remove_unsaved_changes_flag();
    }

    return;
}

void SignalDialog::load_settings(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load video and color settings from...", "",
                                                    "Mode parameters (*.vcs-video *.vcsm);;All files(*.*)");

    if (filename.isEmpty()) return;

    if (kdisk_load_video_signal_parameters(filename.toStdString()))
    {
        remove_unsaved_changes_flag();
    }

    return;
}
