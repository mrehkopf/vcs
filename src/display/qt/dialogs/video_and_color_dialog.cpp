/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS video & color dialog
 *
 * A GUI dialog for allowing the user to adjust the capture card's video and color
 * parameters.
 *
 */

#include <QFileDialog>
#include <QMenuBar>
#include <QDebug>
#include "display/qt/dialogs/video_and_color_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/disk.h"
#include "ui_video_and_color_dialog.h"

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

VideoAndColorDialog::VideoAndColorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoAndColorDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Video & Color");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Set the GUI controls to their proper initial values.
    {
        // Create the dialog's menu bar.
        {
            this->menubar = new QMenuBar(this);

            // File...
            {
                QMenu *fileMenu = new QMenu("File", this);

                fileMenu->addAction("Load settings...", this, SLOT(load_settings()));
                fileMenu->addSeparator();
                fileMenu->addAction("Save settings as...", this, SLOT(save_settings()));

                menubar->addMenu(fileMenu);
            }

            // Help...
            {
                QMenu *fileMenu = new QMenu("Help", this);

                fileMenu->addAction("About...");
                fileMenu->actions().at(0)->setEnabled(false); /// FIXME: Add proper help stuff.

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
                                    this, &VideoAndColorDialog::broadcast_settings);

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
            connect_spinboxes_to_their_sliders(ui->groupBox_colorAdjust);

            update_controls();
        }
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "video_and_color", this->size()).toSize());
    }

    return;
}

VideoAndColorDialog::~VideoAndColorDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "video_and_color", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

// Called to inform the dialog that a new capture signal has been received.
void VideoAndColorDialog::notify_of_new_capture_signal(void)
{
    update_controls();
    update_mode_label(true);

    return;
}

// Called to ask the dialog to poll the capture hardware for its current video
// and color settings.
void VideoAndColorDialog::update_controls(void)
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

void VideoAndColorDialog::set_controls_enabled(const bool state)
{
    ui->groupBox_colorAdjust->setEnabled(state);
    ui->groupBox_videoAdjust->setEnabled(state);
    ui->groupBox_meta->setEnabled(state);

    update_mode_label(state);

    return;
}

void VideoAndColorDialog::update_mode_label(const bool isEnabled)
{
    if (isEnabled)
    {
        const auto signal = kc_capture_api().get_signal_info();
        const QString labelStr = QString("%1 x %2 @ %3 Hz").arg(signal.r.w)
                                                           .arg(signal.r.h)
                                                           .arg(signal.refreshRate);

        ui->label_currentInputMode->setText(labelStr);
    }
    else
    {
        ui->label_currentInputMode->setText("n/a");
    }

    return;
}

// Tally up all the current video and color settings from the GUI, and ask the
// capture card to adopt them.
void VideoAndColorDialog::broadcast_settings(void)
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

    kc_capture_api().assign_video_signal_parameters(p);

    flag_unsaved_changes();

    return;
}

// Activates a user-facing visual indication that there are unsaved changes in
// the dialog.
void VideoAndColorDialog::flag_unsaved_changes(void)
{
    if (!this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().prepend("*"));
    }

    return;
}

// Deactivates the user-facing visual indication that there are unsaved changes
// in the dialog.
void VideoAndColorDialog::remove_unsaved_changes_flag(void)
{
    if (this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().remove(0, 1));
    }

    return;
}

// Called to inform the dialog of a new source file for video and color parameters.
void VideoAndColorDialog::receive_new_mode_settings_filename(const QString &filename)
{
    const QString shortName = QFileInfo(filename).fileName();

    ui->label_settingsFilename->setText(shortName); /// TODO: Elide the filename if it's too long to fit into the label.
    ui->label_settingsFilename->setToolTip(filename);

    return;
}

void VideoAndColorDialog::save_settings(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save video and color settings as...", "",
                                                    "Mode parameters (*.vcs-video *.vcsm);;All files(*.*)");

    if (filename.isEmpty()) return;

    if (!QStringList({"vcs-video", "vcsm"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-video");
    }

    if (kdisk_save_video_signal_parameters(kc_capture_api().get_mode_params(), filename))
    {
        remove_unsaved_changes_flag();
    }

    return;
}

void VideoAndColorDialog::load_settings(void)
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
