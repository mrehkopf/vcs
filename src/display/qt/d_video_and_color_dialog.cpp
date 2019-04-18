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
#include "ui_d_video_and_color_dialog.h"
#include "d_video_and_color_dialog.h"
#include "../../display/qt/persistent_settings.h"
#include "../../capture/capture.h"
#include "../../common/disk.h"
#include "../display.h"
#include "d_util.h"

/*
 * TODOS:
 *
 * - the dialog should communicate more clearly which mode the current GUI controls
 *   are editing, and which modes are available or known about.
 *
 */

// By default, values from the GUI's controls (sliders and spinboxes) will be
// sent to the capture card in real-time, i.e. as the controls are operated.
// Set this to false to disable real-time update.
static bool CONTROLS_LIVE_UPDATE = true;

VideoAndColorDialog::VideoAndColorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoAndColorDialog)
{
    ui->setupUi(this);

    setWindowTitle("VCS - Video and Color Adjust");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    update_controls();

    connect_spinboxes_to_their_sliders(ui->groupBox_videoAdjust);
    connect_spinboxes_to_their_sliders(ui->groupBox_colorAdjust);

    resize(kpers_value_of(INI_GROUP_GEOMETRY, "video_and_color", size()).toSize());

    create_menu_bar();

    return;
}

VideoAndColorDialog::~VideoAndColorDialog()
{
    kpers_set_value(INI_GROUP_GEOMETRY, "video_and_color", size());

    delete ui; ui = nullptr;

    return;
}

void VideoAndColorDialog::create_menu_bar()
{
    k_assert((this->menubar == nullptr), "Not allowed to create the video/color dialog menu bar more than once.");

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

    return;
}

// Call this to notify the dialog of a new capture input signal.
// Fetch the current ranges and values for all of the dialog's controls.
//
void VideoAndColorDialog::notify_of_new_capture_signal(void)
{
    const capture_signal_s s = kc_hardware().status.signal();

    ui->label_currentMode->setText(QString("%1 x %2, %3 Hz").arg(s.r.w).arg(s.r.h).arg(s.refreshRate));

    update_controls();

    return;
}

void VideoAndColorDialog::update_controls(void)
{
    CONTROLS_LIVE_UPDATE = false;
    update_video_controls();
    update_color_controls();
    CONTROLS_LIVE_UPDATE = true;

    flag_unsaved_changes();

    return;
}

void VideoAndColorDialog::set_controls_enabled(const bool state)
{
    ui->groupBox_colorAdjust->setEnabled(state);
    ui->groupBox_videoAdjust->setEnabled(state);
    ui->groupBox_meta->setEnabled(state);

    if (!state)
    {
        ui->label_currentMode->setText("n/a");
    }

    return;
}

void VideoAndColorDialog::update_color_controls()
{
    // Current values.
    {
        const capture_color_settings_s p = kc_hardware().status.color_settings();

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
    }

    // Current valid ranges.
    {
        const capture_color_settings_s min = kc_hardware().meta.minimum_color_settings();
        const capture_color_settings_s max = kc_hardware().meta.maximum_color_settings();

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
    }

    return;
}

void VideoAndColorDialog::update_video_controls()
{
    // Current values.
    {
        const capture_video_settings_s p = kc_hardware().status.video_settings();

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
        const capture_video_settings_s min = kc_hardware().meta.minimum_video_settings();
        const capture_video_settings_s max = kc_hardware().meta.maximum_video_settings();

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

    return;
}

capture_color_settings_s VideoAndColorDialog::current_color_params()
{
    capture_color_settings_s pc;

    pc.overallBrightness = ui->spinBox_colorBright->value();
    pc.overallContrast = ui->spinBox_colorContr->value();
    pc.redBrightness = ui->spinBox_colorBrightRed->value();
    pc.redContrast = ui->spinBox_colorContrRed->value();
    pc.greenBrightness = ui->spinBox_colorBrightGreen->value();
    pc.greenContrast = ui->spinBox_colorContrGreen->value();
    pc.blueBrightness = ui->spinBox_colorBrightBlue->value();
    pc.blueContrast = ui->spinBox_colorContrBlue->value();

    return pc;
}

capture_video_settings_s VideoAndColorDialog::current_video_params()
{
    capture_video_settings_s pv;

    pv.blackLevel = ui->spinBox_videoBlackLevel->value();
    pv.horizontalPosition = ui->spinBox_videoHorPos->value();
    pv.horizontalScale = ui->spinBox_videoHorSize->value();
    pv.phase = ui->spinBox_videoPhase->value();
    pv.verticalPosition = ui->spinBox_videoVerPos->value();

    return pv;
}

// Loops through each spinbox to find the corresponding slider to connect their
// valueChanged signals.
//
void VideoAndColorDialog::connect_spinboxes_to_their_sliders(QGroupBox *const parent)
{
    for (int i = 0; i < parent->layout()->count(); i++)
    {
        QWidget *const spinbox = parent->layout()->itemAt(i)->widget();

        if (spinbox == nullptr ||
            !spinbox->objectName().startsWith("spinBox_"))
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
                auto spinBoxValueChanged = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged); // To connect to overloaded valueChanged(int).

                connect((QSpinBox*)spinbox, spinBoxValueChanged,
                        (QScrollBar*)slider, &QScrollBar::setValue);

                connect((QScrollBar*)slider, &QScrollBar::valueChanged,
                        (QSpinBox*)spinbox, &QSpinBox::setValue);

                connect((QSpinBox*)spinbox, spinBoxValueChanged,
                        this, &VideoAndColorDialog::broadcast_settings_change);

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
}

// Sends all of the dialog's video and color options to the capture card.
//
void VideoAndColorDialog::broadcast_settings_change()
{
    const capture_video_settings_s vp = current_video_params();
    const capture_color_settings_s cp = current_color_params();

    if (!CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    flag_unsaved_changes();

    kc_set_video_settings(vp);
    kc_set_color_settings(cp);

    return;
}

// Call this to give the user a visual indication that there are unsaved changes
// in the settings.
//
void VideoAndColorDialog::flag_unsaved_changes(void)
{
    const QString filenameString = ui->label_settingsFilename->text();

    if (!filenameString.startsWith("*"))
    {
        ui->label_settingsFilename->setText("*" + filenameString);
    }

    return;
}

// Call this to inform the dialog of a new source file for mode parameters.
//
void VideoAndColorDialog::receive_new_mode_settings_filename(const QString &filename)
{
    const QString shortName = QFileInfo(filename).fileName();

    ui->label_settingsFilename->setText(shortName); /// TODO: Elide the filename if it's too long to fit into the label.
    ui->label_settingsFilename->setToolTip(filename);
}

void VideoAndColorDialog::save_settings(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save video and color settings as...", "",
                                                    "Mode parameters (*.vcs-video *.vcsm);;All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    if (!QStringList({"vcs-video", "vcsm"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-video");
    }

    kdisk_save_mode_params(kc_mode_params(), filename);

    return;
}

void VideoAndColorDialog::load_settings(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load video and color settings from...", "",
                                                    "Mode parameters (*.vcs-video *.vcsm);;All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    kdisk_load_mode_params(filename.toStdString());

    return;
}
