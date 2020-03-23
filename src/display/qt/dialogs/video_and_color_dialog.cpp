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
        const capture_color_settings_s colorParams = kc_capture_api().get_color_settings();

        ui->spinBox_colorBright->setValue(colorParams.overallBrightness);
        ui->horizontalScrollBar_colorBright->setValue(colorParams.overallBrightness);

        ui->spinBox_colorContr->setValue(colorParams.overallContrast);
        ui->horizontalScrollBar_colorContr->setValue(colorParams.overallContrast);

        ui->spinBox_colorBrightRed->setValue(colorParams.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setValue(colorParams.redBrightness);

        ui->spinBox_colorContrRed->setValue(colorParams.redContrast);
        ui->horizontalScrollBar_colorContrRed->setValue(colorParams.redContrast);

        ui->spinBox_colorBrightGreen->setValue(colorParams.greenBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setValue(colorParams.greenBrightness);

        ui->spinBox_colorContrGreen->setValue(colorParams.greenContrast);
        ui->horizontalScrollBar_colorContrGreen->setValue(colorParams.greenContrast);

        ui->spinBox_colorBrightBlue->setValue(colorParams.blueBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setValue(colorParams.blueBrightness);

        ui->spinBox_colorContrBlue->setValue(colorParams.blueContrast);
        ui->horizontalScrollBar_colorContrBlue->setValue(colorParams.blueContrast);

        const capture_video_settings_s videoParams = kc_capture_api().get_video_settings();

        ui->spinBox_videoBlackLevel->setValue(videoParams.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setValue(videoParams.blackLevel);

        ui->spinBox_videoHorPos->setValue(videoParams.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setValue(videoParams.horizontalPosition);

        ui->spinBox_videoHorSize->setValue(videoParams.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setValue(videoParams.horizontalScale);

        ui->spinBox_videoPhase->setValue(videoParams.phase);
        ui->horizontalScrollBar_videoPhase->setValue(videoParams.phase);

        ui->spinBox_videoVerPos->setValue(videoParams.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setValue(videoParams.verticalPosition);
    }

    // Current valid ranges.
    {
        const capture_color_settings_s minColorParams = kc_capture_api().get_minimum_color_settings();
        const capture_color_settings_s maxColorParams = kc_capture_api().get_maximum_color_settings();

        ui->spinBox_colorBright->setMinimum(minColorParams.overallBrightness);
        ui->spinBox_colorBright->setMaximum(maxColorParams.overallBrightness);
        ui->horizontalScrollBar_colorBright->setMinimum(minColorParams.overallBrightness);
        ui->horizontalScrollBar_colorBright->setMaximum(maxColorParams.overallBrightness);

        ui->spinBox_colorContr->setMinimum(minColorParams.overallContrast);
        ui->spinBox_colorContr->setMaximum(maxColorParams.overallContrast);
        ui->horizontalScrollBar_colorContr->setMinimum(minColorParams.overallContrast);
        ui->horizontalScrollBar_colorContr->setMaximum(maxColorParams.overallContrast);

        ui->spinBox_colorBrightRed->setMinimum(minColorParams.redBrightness);
        ui->spinBox_colorBrightRed->setMaximum(maxColorParams.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setMinimum(minColorParams.redBrightness);
        ui->horizontalScrollBar_colorBrightRed->setMaximum(maxColorParams.redBrightness);

        ui->spinBox_colorContrRed->setMinimum(minColorParams.redContrast);
        ui->spinBox_colorContrRed->setMaximum(maxColorParams.redContrast);
        ui->horizontalScrollBar_colorContrRed->setMinimum(minColorParams.redContrast);
        ui->horizontalScrollBar_colorContrRed->setMaximum(maxColorParams.redContrast);

        ui->spinBox_colorBrightGreen->setMinimum(minColorParams.blueBrightness);
        ui->spinBox_colorBrightGreen->setMaximum(maxColorParams.blueBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setMinimum(minColorParams.blueBrightness);
        ui->horizontalScrollBar_colorBrightGreen->setMaximum(maxColorParams.blueBrightness);

        ui->spinBox_colorContrGreen->setMinimum(minColorParams.blueContrast);
        ui->spinBox_colorContrGreen->setMaximum(maxColorParams.blueContrast);
        ui->horizontalScrollBar_colorContrGreen->setMinimum(minColorParams.blueContrast);
        ui->horizontalScrollBar_colorContrGreen->setMaximum(maxColorParams.blueContrast);

        ui->spinBox_colorBrightBlue->setMinimum(minColorParams.greenBrightness);
        ui->spinBox_colorBrightBlue->setMaximum(maxColorParams.greenBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setMinimum(minColorParams.greenBrightness);
        ui->horizontalScrollBar_colorBrightBlue->setMaximum(maxColorParams.greenBrightness);

        ui->spinBox_colorContrBlue->setMinimum(minColorParams.greenContrast);
        ui->spinBox_colorContrBlue->setMaximum(maxColorParams.greenContrast);
        ui->horizontalScrollBar_colorContrBlue->setMinimum(minColorParams.greenContrast);
        ui->horizontalScrollBar_colorContrBlue->setMaximum(maxColorParams.greenContrast);

        const capture_video_settings_s minVideoParams = kc_capture_api().get_minimum_video_settings();
        const capture_video_settings_s maxVideoParams = kc_capture_api().get_maximum_video_settings();

        ui->spinBox_videoBlackLevel->setMinimum(minVideoParams.blackLevel);
        ui->spinBox_videoBlackLevel->setMaximum(maxVideoParams.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMinimum(minVideoParams.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMaximum(maxVideoParams.blackLevel);

        ui->spinBox_videoHorPos->setMinimum(minVideoParams.horizontalPosition);
        ui->spinBox_videoHorPos->setMaximum(maxVideoParams.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setMinimum(minVideoParams.horizontalPosition);
        ui->horizontalScrollBar_videoHorPos->setMaximum(maxVideoParams.horizontalPosition);

        ui->spinBox_videoHorSize->setMinimum(minVideoParams.horizontalScale);
        ui->spinBox_videoHorSize->setMaximum(maxVideoParams.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setMinimum(minVideoParams.horizontalScale);
        ui->horizontalScrollBar_videoHorSize->setMaximum(maxVideoParams.horizontalScale);

        ui->spinBox_videoPhase->setMinimum(minVideoParams.phase);
        ui->spinBox_videoPhase->setMaximum(maxVideoParams.phase);
        ui->horizontalScrollBar_videoPhase->setMinimum(minVideoParams.phase);
        ui->horizontalScrollBar_videoPhase->setMaximum(maxVideoParams.phase);

        ui->spinBox_videoVerPos->setMinimum(minVideoParams.verticalPosition);
        ui->spinBox_videoVerPos->setMaximum(maxVideoParams.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setMinimum(minVideoParams.verticalPosition);
        ui->horizontalScrollBar_videoVerPos->setMaximum(maxVideoParams.verticalPosition);
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

    capture_color_settings_s currentColorParams;
    currentColorParams.overallBrightness = ui->spinBox_colorBright->value();
    currentColorParams.overallContrast = ui->spinBox_colorContr->value();
    currentColorParams.redBrightness = ui->spinBox_colorBrightRed->value();
    currentColorParams.redContrast = ui->spinBox_colorContrRed->value();
    currentColorParams.greenBrightness = ui->spinBox_colorBrightGreen->value();
    currentColorParams.greenContrast = ui->spinBox_colorContrGreen->value();
    currentColorParams.blueBrightness = ui->spinBox_colorBrightBlue->value();
    currentColorParams.blueContrast = ui->spinBox_colorContrBlue->value();

    capture_video_settings_s currentVideoParams;
    currentVideoParams.blackLevel = ui->spinBox_videoBlackLevel->value();
    currentVideoParams.horizontalPosition = ui->spinBox_videoHorPos->value();
    currentVideoParams.horizontalScale = ui->spinBox_videoHorSize->value();
    currentVideoParams.phase = ui->spinBox_videoPhase->value();
    currentVideoParams.verticalPosition = ui->spinBox_videoVerPos->value();

    kc_capture_api().set_video_settings(currentVideoParams);
    kc_capture_api().set_color_settings(currentColorParams);

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

    if (kdisk_save_video_mode_params(kc_capture_api().get_mode_params(), filename))
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

    if (kdisk_load_video_mode_params(filename.toStdString()))
    {
        remove_unsaved_changes_flag();
    }

    return;
}
