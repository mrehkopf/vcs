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
#include "../../common/persistent_settings.h"
#include "../../capture/capture.h"
#include "../display.h"
#include "../../main.h"
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
    clear_known_modes();

    connect_spinboxes_to_their_sliders(ui->groupBox_videoAdjust);
    connect_spinboxes_to_their_sliders(ui->groupBox_colorAdjust);

    resize(kpers_value_of("video_and_color", INI_GROUP_GEOMETRY, size()).toSize());

    create_menu_bar();

    return;
}

VideoAndColorDialog::~VideoAndColorDialog()
{
    kpers_set_value("video_and_color", INI_GROUP_GEOMETRY, size());

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
void VideoAndColorDialog::receive_new_input_signal(const input_signal_s &s)
{
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
        const input_color_params_s p = kc_capture_color_params();

        ui->spinBox_colorBright->setValue(p.bright);
        ui->horizontalScrollBar_colorBright->setValue(p.bright);

        ui->spinBox_colorContr->setValue(p.contr);
        ui->horizontalScrollBar_colorContr->setValue(p.contr);

        ui->spinBox_colorBrightRed->setValue(p.redBright);
        ui->horizontalScrollBar_colorBrightRed->setValue(p.redBright);

        ui->spinBox_colorContrRed->setValue(p.redContr);
        ui->horizontalScrollBar_colorContrRed->setValue(p.redContr);

        ui->spinBox_colorBrightGreen->setValue(p.greenBright);
        ui->horizontalScrollBar_colorBrightGreen->setValue(p.greenBright);

        ui->spinBox_colorContrGreen->setValue(p.greenContr);
        ui->horizontalScrollBar_colorContrGreen->setValue(p.greenContr);

        ui->spinBox_colorBrightBlue->setValue(p.blueBright);
        ui->horizontalScrollBar_colorBrightBlue->setValue(p.blueBright);

        ui->spinBox_colorContrBlue->setValue(p.blueContr);
        ui->horizontalScrollBar_colorContrBlue->setValue(p.blueContr);
    }

    // Current valid ranges.
    {
        const input_color_params_s min = kc_capture_color_params_min_values();
        const input_color_params_s max = kc_capture_color_params_max_values();

        ui->spinBox_colorBright->setMinimum(min.bright);
        ui->spinBox_colorBright->setMaximum(max.bright);
        ui->horizontalScrollBar_colorBright->setMinimum(min.bright);
        ui->horizontalScrollBar_colorBright->setMaximum(max.bright);

        ui->spinBox_colorContr->setMinimum(min.contr);
        ui->spinBox_colorContr->setMaximum(max.contr);
        ui->horizontalScrollBar_colorContr->setMinimum(min.contr);
        ui->horizontalScrollBar_colorContr->setMaximum(max.contr);

        ui->spinBox_colorBrightRed->setMinimum(min.redBright);
        ui->spinBox_colorBrightRed->setMaximum(max.redBright);
        ui->horizontalScrollBar_colorBrightRed->setMinimum(min.redBright);
        ui->horizontalScrollBar_colorBrightRed->setMaximum(max.redBright);

        ui->spinBox_colorContrRed->setMinimum(min.redContr);
        ui->spinBox_colorContrRed->setMaximum(max.redContr);
        ui->horizontalScrollBar_colorContrRed->setMinimum(min.redContr);
        ui->horizontalScrollBar_colorContrRed->setMaximum(max.redContr);

        ui->spinBox_colorBrightGreen->setMinimum(min.blueBright);
        ui->spinBox_colorBrightGreen->setMaximum(max.blueBright);
        ui->horizontalScrollBar_colorBrightGreen->setMinimum(min.blueBright);
        ui->horizontalScrollBar_colorBrightGreen->setMaximum(max.blueBright);

        ui->spinBox_colorContrGreen->setMinimum(min.blueContr);
        ui->spinBox_colorContrGreen->setMaximum(max.blueContr);
        ui->horizontalScrollBar_colorContrGreen->setMinimum(min.blueContr);
        ui->horizontalScrollBar_colorContrGreen->setMaximum(max.blueContr);

        ui->spinBox_colorBrightBlue->setMinimum(min.greenBright);
        ui->spinBox_colorBrightBlue->setMaximum(max.greenBright);
        ui->horizontalScrollBar_colorBrightBlue->setMinimum(min.greenBright);
        ui->horizontalScrollBar_colorBrightBlue->setMaximum(max.greenBright);

        ui->spinBox_colorContrBlue->setMinimum(min.greenContr);
        ui->spinBox_colorContrBlue->setMaximum(max.greenContr);
        ui->horizontalScrollBar_colorContrBlue->setMinimum(min.greenContr);
        ui->horizontalScrollBar_colorContrBlue->setMaximum(max.greenContr);
    }

    return;
}

void VideoAndColorDialog::update_video_controls()
{
    // Current values.
    {
        const input_video_params_s p = kc_capture_video_params();

        ui->spinBox_videoBlackLevel->setValue(p.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setValue(p.blackLevel);

        ui->spinBox_videoHorPos->setValue(p.horPos);
        ui->horizontalScrollBar_videoHorPos->setValue(p.horPos);

        ui->spinBox_videoHorSize->setValue(p.horScale);
        ui->horizontalScrollBar_videoHorSize->setValue(p.horScale);

        ui->spinBox_videoPhase->setValue(p.phase);
        ui->horizontalScrollBar_videoPhase->setValue(p.phase);

        ui->spinBox_videoVerPos->setValue(p.verPos);
        ui->horizontalScrollBar_videoVerPos->setValue(p.verPos);
    }

    // Current valid ranges.
    {
        const input_video_params_s min = kc_capture_video_params_min_values();
        const input_video_params_s max = kc_capture_video_params_max_values();

        ui->spinBox_videoBlackLevel->setMinimum(min.blackLevel);
        ui->spinBox_videoBlackLevel->setMaximum(max.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMinimum(min.blackLevel);
        ui->horizontalScrollBar_videoBlackLevel->setMaximum(max.blackLevel);

        ui->spinBox_videoHorPos->setMinimum(min.horPos);
        ui->spinBox_videoHorPos->setMaximum(max.horPos);
        ui->horizontalScrollBar_videoHorPos->setMinimum(min.horPos);
        ui->horizontalScrollBar_videoHorPos->setMaximum(max.horPos);

        ui->spinBox_videoHorSize->setMinimum(min.horScale);
        ui->spinBox_videoHorSize->setMaximum(max.horScale);
        ui->horizontalScrollBar_videoHorSize->setMinimum(min.horScale);
        ui->horizontalScrollBar_videoHorSize->setMaximum(max.horScale);

        ui->spinBox_videoPhase->setMinimum(min.phase);
        ui->spinBox_videoPhase->setMaximum(max.phase);
        ui->horizontalScrollBar_videoPhase->setMinimum(min.phase);
        ui->horizontalScrollBar_videoPhase->setMaximum(max.phase);

        ui->spinBox_videoVerPos->setMinimum(min.verPos);
        ui->spinBox_videoVerPos->setMaximum(max.verPos);
        ui->horizontalScrollBar_videoVerPos->setMinimum(min.verPos);
        ui->horizontalScrollBar_videoVerPos->setMaximum(max.verPos);
    }

    return;
}

input_color_params_s VideoAndColorDialog::current_color_params()
{
    input_color_params_s pc;

    pc.bright = ui->spinBox_colorBright->value();
    pc.contr = ui->spinBox_colorContr->value();
    pc.redBright = ui->spinBox_colorBrightRed->value();
    pc.redContr = ui->spinBox_colorContrRed->value();
    pc.greenBright = ui->spinBox_colorBrightGreen->value();
    pc.greenContr = ui->spinBox_colorContrGreen->value();
    pc.blueBright = ui->spinBox_colorBrightBlue->value();
    pc.blueContr = ui->spinBox_colorContrBlue->value();

    return pc;
}

input_video_params_s VideoAndColorDialog::current_video_params()
{
    input_video_params_s pv;

    pv.blackLevel = ui->spinBox_videoBlackLevel->value();
    pv.horPos = ui->spinBox_videoHorPos->value();
    pv.horScale = ui->spinBox_videoHorSize->value();
    pv.phase = ui->spinBox_videoPhase->value();
    pv.verPos = ui->spinBox_videoVerPos->value();

    return pv;
}

// Clear the list of known video modes.
//
void VideoAndColorDialog::clear_known_modes()
{
#if 0 // Disabled while the UI is being reworked.
    { block_widget_signals_c b(ui->comboBox_knownModes);
        ui->comboBox_knownModes->clear();
        ui->comboBox_knownModes->addItem("Custom parameters are available for:");   /// Temp hack.
    }
#endif

    return;
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
    const input_video_params_s vp = current_video_params();
    const input_color_params_s cp = current_color_params();

    if (!CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    flag_unsaved_changes();

    kc_set_capture_video_params(vp);
    kc_set_capture_color_params(cp);

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
                                                    "Mode parameters (*.vcsm);;All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    if (QFileInfo(filename).suffix() != "vcsm") filename.append(".vcsm");

    kc_save_mode_params(filename);

    return;
}

void VideoAndColorDialog::load_settings(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load video and color settings from...", "",
                                                    "Mode parameters (*.vcsm);;All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    kc_load_mode_params(filename, true);

    return;
}
