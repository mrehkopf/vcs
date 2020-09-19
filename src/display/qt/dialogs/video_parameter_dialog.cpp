#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/refresh_rate.h"
#include "common/disk/disk.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "capture/video_presets.h"
#include "ui_video_parameter_dialog.h"

// By default, values from the GUI's controls (sliders and spinboxes) will be
// sent to the capture card in real-time, i.e. as the controls are operated.
// Set this variable to false to disable real-time updates.
static bool CONTROLS_LIVE_UPDATE = true;

VideoParameterDialog::VideoParameterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoParameterDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(this->dialogBaseTitle);

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Video parameters...
        {
            QMenu *presetMenu = new QMenu("Presets", this->menubar);

            QAction *addPreset = new QAction("Add new", presetMenu);
            QAction *savePresets = new QAction("Save as...", presetMenu);
            QAction *loadPresets = new QAction("Load...", presetMenu);
            QAction *deletePreset = new QAction("Delete selected", presetMenu);
            QAction *deleteAllPresets = new QAction("Delete all", presetMenu);

            presetMenu->addAction(addPreset);
            presetMenu->addSeparator();
            presetMenu->addAction(deleteAllPresets);
            presetMenu->addAction(deletePreset);
            presetMenu->addSeparator();
            presetMenu->addAction(loadPresets);
            presetMenu->addAction(savePresets);

            connect(this, &VideoParameterDialog::preset_list_became_empty, this, [=]
            {
                savePresets->setEnabled(false);
                deletePreset->setEnabled(false);
                deleteAllPresets->setEnabled(false);
            });

            connect(this, &VideoParameterDialog::preset_list_no_longer_empty, this, [=]
            {
                savePresets->setEnabled(true);
                deletePreset->setEnabled(true);
                deleteAllPresets->setEnabled(true);
            });

            connect(addPreset, &QAction::triggered, this, [=]
            {
                this->add_video_preset_to_list(kvideopreset_create_preset());
            });

            connect(savePresets, &QAction::triggered, this, [=]
            {
                QString filename = QFileDialog::getSaveFileName(this, "Save video presets as...", "",
                                                                "Video presets (*.vcs-video);;All files (*.*)");

                if (filename.isEmpty())
                {
                    return;
                }

                if (!QStringList({"vcs-video"}).contains(QFileInfo(filename).suffix()))
                {
                    filename.append(".vcs-video");
                }

                if (kdisk_save_video_presets(kvideopreset_all_presets(), filename.toStdString()))
                {
                    /// TODO: remove_unsaved_changes_flag();
                }
            });

            connect(loadPresets, &QAction::triggered, this, [=]
            {
                QString filename = QFileDialog::getOpenFileName(this, "Load video presets from...", "",
                                                                "Video presets (*.vcs-video);;All files(*.*)");

                if (filename.isEmpty())
                {
                    return;
                }

                if (kdisk_load_video_presets(filename.toStdString()))
                {
                    /// TODO: remove_unsaved_changes_flag();
                }
            });

            connect(deletePreset, &QAction::triggered, this, [=]
            {
                if (this->currentPreset &&
                    (QMessageBox::question(this, "Confirm preset deletion",
                                           "Do you want to delete this preset?",
                                           (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes))
                {
                    const auto presetToDelete = this->currentPreset;
                    this->remove_video_preset_from_list(presetToDelete);
                    kvideopreset_remove_preset(presetToDelete->id);
                    kvideopreset_apply_current_active_preset();
                }
            });

            connect(deleteAllPresets, &QAction::triggered, this, [=]
            {
                if (QMessageBox::warning(this, "Confirm deletion of all presets",
                                          "Do you really want to delete ALL presets?",
                                          (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes)
                {
                    this->remove_all_video_presets_from_list();
                    kvideopreset_remove_all_presets();
                    kvideopreset_apply_current_active_preset();
                }
            });

            this->menubar->addMenu(presetMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Set the GUI controls to their proper initial values.
    {
        // The file format we save presets into reserves the { and } characters;
        // these characters shouldn't occur in preset names.
        ui->lineEdit_presetName->setValidator(new QRegExpValidator(QRegExp("[^{}]*"), this));

        ui->groupBox_activation->setEnabled(false);
        ui->groupBox_presetList->setEnabled(false);
        ui->groupBox_presetName->setEnabled(false);
        ui->groupBox_videoParameters->setEnabled(false);

        const auto minres = kc_capture_api().get_minimum_resolution();
        const auto maxres = kc_capture_api().get_maximum_resolution();

        ui->spinBox_resolutionX->setMinimum(int(minres.w));
        ui->spinBox_resolutionX->setMaximum(int(maxres.w));
        ui->spinBox_resolutionY->setMinimum(int(minres.w));
        ui->spinBox_resolutionY->setMaximum(int(maxres.w));
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        #define OVERLOAD_DOUBLE static_cast<void (QDoubleSpinBox::*)(double)>

        connect(ui->doubleSpinBox_refreshRateValue, OVERLOAD_DOUBLE(&QDoubleSpinBox::valueChanged), this, [this](const double value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationRefreshRate = value;
                this->update_current_present_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_DOUBLE

        #define OVERLOAD_INT static_cast<void (QSpinBox::*)(int)>

        connect(ui->spinBox_resolutionX, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this](const int value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationResolution.w = std::min(MAX_OUTPUT_WIDTH, unsigned(value));
                this->update_current_present_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        connect(ui->spinBox_resolutionY, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this](const int value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationResolution.h = std::min(MAX_OUTPUT_HEIGHT, unsigned(value));
                this->update_current_present_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_INT

        connect(ui->comboBox_presetList, &QComboBox::currentTextChanged, this, [this]
        {
            // Sort the list alphabetically.
            ui->comboBox_presetList->model()->sort(0);
        });

        connect(ui->lineEdit_presetName, &QLineEdit::textEdited, this, [this](const QString &text)
        {
            if (this->currentPreset)
            {
                this->currentPreset->name = text.toStdString();
                this->update_current_present_list_text();
            }
        });

        connect(ui->checkBox_activatorRefreshRate, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->doubleSpinBox_refreshRateValue->setEnabled(checked);
            ui->comboBox_refreshRateComparison->setEnabled(checked);
            ui->label_refreshRateSeparator->setEnabled(checked);

            if (this->currentPreset)
            {
                this->currentPreset->activatesWithRefreshRate = checked;
                this->update_current_present_list_text();
            }

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->checkBox_activatorResolution, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->spinBox_resolutionX->setEnabled(checked);
            ui->spinBox_resolutionY->setEnabled(checked);
            ui->label_resolutionSeparator->setEnabled(checked);

            if (this->currentPreset)
            {
                this->currentPreset->activatesWithResolution = checked;
                this->update_current_present_list_text();
            }

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->checkBox_activatorShortcut, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->comboBox_shortcutFirstKey->setEnabled(checked);
            ui->comboBox_shortcutSecondKey->setEnabled(checked);
            ui->label_shortcutSeparator->setEnabled(checked);

            if (this->currentPreset)
            {
                this->currentPreset->activatesWithShortcut = checked;
                this->update_current_present_list_text();
            }
        });

        // The valueChanged() signal is overloaded for int and QString, and we
        // have to choose one. I'm using Qt 5.5, but you may have better ways
        // of doing this in later versions.
        #define OVERLOAD_QSTRING static_cast<void (QComboBox::*)(const QString&)>

        connect(ui->comboBox_shortcutSecondKey, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this](const QString &text)
        {
            if (this->currentPreset)
            {
                QString newShortcut = QString("ctrl+%1").arg(text);
                this->currentPreset->activationShortcut = newShortcut.toStdString();

                this->update_current_present_list_text();
            }

            this->update_preset_controls();
        });

        connect(ui->comboBox_presetList, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this]()
        {
            if (ui->comboBox_presetList->count() > 0)
            {
                this->update_preset_controls();
                this->update_current_present_list_text();
            }

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->comboBox_refreshRateComparison, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this](const QString &text)
        {
            if (text == "Exact") ui->doubleSpinBox_refreshRateValue->setDecimals(refresh_rate_s::numDecimalsPrecision);
            else ui->doubleSpinBox_refreshRateValue->setDecimals(0);

            if (this->currentPreset)
            {
                if (text == "Exact")   this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::equals;
                if (text == "Rounded") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::rounded;
                if (text == "Floored") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::floored;
                if (text == "Ceiled")  this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::ceiled;

                this->update_current_present_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_QSTRING

        // Connect sliders to their corresponding spin boxes.
        {
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

                            connect((QSpinBox*)spinbox, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this]
                            {
                                this->broadcast_current_preset_parameters();
                            });

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

            connect_spinboxes_to_their_sliders(ui->groupBox_videoParameters);
        }
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "video_parameters", this->size()).toSize());
    }

    // Force initialization of any GUI elements that respond to dynamic changes
    // in the preset list.
    if (ui->comboBox_presetList->count() <= 0)
    {
        emit this->preset_list_became_empty();
    }
    else
    {
        emit this->preset_list_no_longer_empty();
    }

    return;
}

VideoParameterDialog::~VideoParameterDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "video_parameters", this->size());
    }

    delete ui;

    return;
}

void VideoParameterDialog::receive_new_video_presets_filename(const QString &filename)
{
    const QString newWindowTitle = QString("%1%2 - %3").arg(this->windowTitle().startsWith("*")? "*" : "")
                                                       .arg(this->dialogBaseTitle)
                                                       .arg(QFileInfo(filename).baseName());

    this->remove_all_video_presets_from_list();

    for (const auto *p: kvideopreset_all_presets())
    {
        this->add_video_preset_to_list(p);
    }

    this->setWindowTitle(newWindowTitle);

    return;
}

void VideoParameterDialog::update_current_present_list_text(void)
{
    ui->comboBox_presetList->setItemText(ui->comboBox_presetList->currentIndex(), this->make_preset_list_text(this->currentPreset));

    return;
}

void VideoParameterDialog::broadcast_current_preset_parameters(void)
{
    if (!this->currentPreset ||
        !CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    this->currentPreset->videoParameters.overallBrightness  = ui->spinBox_colorBright->value();
    this->currentPreset->videoParameters.overallContrast    = ui->spinBox_colorContr->value();
    this->currentPreset->videoParameters.redBrightness      = ui->spinBox_colorBrightRed->value();
    this->currentPreset->videoParameters.redContrast        = ui->spinBox_colorContrRed->value();
    this->currentPreset->videoParameters.greenBrightness    = ui->spinBox_colorBrightGreen->value();
    this->currentPreset->videoParameters.greenContrast      = ui->spinBox_colorContrGreen->value();
    this->currentPreset->videoParameters.blueBrightness     = ui->spinBox_colorBrightBlue->value();
    this->currentPreset->videoParameters.blueContrast       = ui->spinBox_colorContrBlue->value();
    this->currentPreset->videoParameters.blackLevel         = ui->spinBox_videoBlackLevel->value();
    this->currentPreset->videoParameters.horizontalPosition = ui->spinBox_videoHorPos->value();
    this->currentPreset->videoParameters.horizontalScale    = ui->spinBox_videoHorSize->value();
    this->currentPreset->videoParameters.phase              = ui->spinBox_videoPhase->value();
    this->currentPreset->videoParameters.verticalPosition   = ui->spinBox_videoVerPos->value();

    kvideoparam_preset_video_params_changed(this->currentPreset->id);

    return;
}

QString VideoParameterDialog::make_preset_list_text(const video_preset_s *const preset)
{
    QStringList text;

    if (preset->activatesWithShortcut)
    {
        text << QString("[Ctrl+F%1]").arg(QString::number(ui->comboBox_shortcutSecondKey->currentIndex() + 1));
    }

    if (preset->activatesWithResolution)
    {
        text << QString("%1 x %2").arg(QString::number(preset->activationResolution.w))
                                  .arg(QString::number(preset->activationResolution.h));
    }

    if (preset->activatesWithRefreshRate)
    {
        if (preset->refreshRateComparator == video_preset_s::refresh_rate_comparison_e::equals)
        {
            text << QString("%1 Hz").arg(QString::number(preset->activationRefreshRate.value<double>(), 'f', 3));
        }
        else
        {
            text << QString("%1 Hz").arg(QString::number(preset->activationRefreshRate.value<double>()));
        }
    }

    if (!preset->name.empty())
    {
        text << QString::fromStdString(preset->name);
    }

    return text.join(" - ");
}

void VideoParameterDialog::remove_all_video_presets_from_list(void)
{
    if (ui->comboBox_presetList->count() <= 0)
    {
        return;
    }

    ui->groupBox_activation->setEnabled(false);
    ui->groupBox_presetList->setEnabled(false);
    ui->groupBox_presetName->setEnabled(false);
    ui->groupBox_videoParameters->setEnabled(false);
    ui->lineEdit_presetName->clear();

    this->currentPreset = nullptr;

    ui->comboBox_presetList->clear();

    emit this->preset_list_became_empty();

    return;
}

void VideoParameterDialog::remove_video_preset_from_list(const video_preset_s *const preset)
{
    if (ui->comboBox_presetList->count() <= 0)
    {
        return;
    }

    // If this is the last preset to be removed, disable the dialog's preset
    // controls.
    if ((ui->comboBox_presetList->count() == 1))
    {
        ui->groupBox_activation->setEnabled(false);
        ui->groupBox_presetList->setEnabled(false);
        ui->groupBox_presetName->setEnabled(false);
        ui->groupBox_videoParameters->setEnabled(false);
        ui->lineEdit_presetName->clear();

        this->currentPreset = nullptr;

        emit this->preset_list_became_empty();
    }

    const int presetListIdx = ui->comboBox_presetList->findText(this->make_preset_list_text(preset));

    k_assert((presetListIdx >= 0), "Invalid list index.");

    ui->comboBox_presetList->removeItem(presetListIdx);
}

void VideoParameterDialog::add_video_preset_to_list(const video_preset_s *const preset)
{
    // If this is the first preset to the added to an empty list, enable the
    // dialog's preset controls - which will have been disabled while there were
    // no presets in the list.
    if (ui->comboBox_presetList->count() == 0)
    {
        ui->groupBox_activation->setEnabled(true);
        ui->groupBox_presetList->setEnabled(true);
        ui->groupBox_presetName->setEnabled(true);
        ui->groupBox_videoParameters->setEnabled(true);

        emit this->preset_list_no_longer_empty();
    }

    const QString presetText = this->make_preset_list_text(preset);

    ui->comboBox_presetList->addItem(presetText, preset->id);

    // Sort the list alphabetically.
    ui->comboBox_presetList->model()->sort(0);

    ui->comboBox_presetList->setCurrentText(presetText);

    return;
}

void VideoParameterDialog::update_preset_control_ranges(void)
{
    video_signal_parameters_s currentParams = {};

    if (this->currentPreset)
    {
        currentParams = this->currentPreset->videoParameters;
    }

    // Current valid ranges. Note: we adjust the min/max values so that Qt
    // doesn't clip preset data that falls outside of the capture card's
    // min/max range.
    {
        const video_signal_parameters_s min = kc_capture_api().get_minimum_video_signal_parameters();
        const video_signal_parameters_s max = kc_capture_api().get_maximum_video_signal_parameters();

        ui->spinBox_colorBright->setMinimum(std::min(currentParams.overallBrightness, min.overallBrightness));
        ui->spinBox_colorBright->setMaximum(std::max(currentParams.overallBrightness, max.overallBrightness));
        ui->horizontalScrollBar_colorBright->setMinimum(ui->spinBox_colorBright->minimum());
        ui->horizontalScrollBar_colorBright->setMaximum(ui->spinBox_colorBright->maximum());

        ui->spinBox_colorContr->setMinimum(std::min(currentParams.overallContrast, min.overallContrast));
        ui->spinBox_colorContr->setMaximum(std::max(currentParams.overallContrast, max.overallContrast));
        ui->horizontalScrollBar_colorContr->setMinimum(ui->spinBox_colorContr->minimum());
        ui->horizontalScrollBar_colorContr->setMaximum(ui->spinBox_colorContr->maximum());

        ui->spinBox_colorBrightRed->setMinimum(std::min(currentParams.redBrightness, min.redBrightness));
        ui->spinBox_colorBrightRed->setMaximum(std::max(currentParams.redBrightness, max.redBrightness));
        ui->horizontalScrollBar_colorBrightRed->setMinimum(ui->spinBox_colorBrightRed->minimum());
        ui->horizontalScrollBar_colorBrightRed->setMaximum(ui->spinBox_colorBrightRed->maximum());

        ui->spinBox_colorContrRed->setMinimum(std::min(currentParams.redContrast, min.redContrast));
        ui->spinBox_colorContrRed->setMaximum(std::max(currentParams.redContrast, max.redContrast));
        ui->horizontalScrollBar_colorContrRed->setMinimum(ui->spinBox_colorContrRed->minimum());
        ui->horizontalScrollBar_colorContrRed->setMaximum(ui->spinBox_colorContrRed->maximum());

        ui->spinBox_colorBrightGreen->setMinimum(std::min(currentParams.greenBrightness, min.greenBrightness));
        ui->spinBox_colorBrightGreen->setMaximum(std::max(currentParams.greenBrightness, max.greenBrightness));
        ui->horizontalScrollBar_colorBrightGreen->setMinimum(ui->spinBox_colorBrightGreen->minimum());
        ui->horizontalScrollBar_colorBrightGreen->setMaximum(ui->spinBox_colorBrightGreen->maximum());

        ui->spinBox_colorContrGreen->setMinimum(std::min(currentParams.greenContrast, min.greenContrast));
        ui->spinBox_colorContrGreen->setMaximum(std::max(currentParams.greenContrast, max.greenContrast));
        ui->horizontalScrollBar_colorContrGreen->setMinimum(ui->spinBox_colorContrGreen->minimum());
        ui->horizontalScrollBar_colorContrGreen->setMaximum(ui->spinBox_colorContrGreen->maximum());

        ui->spinBox_colorBrightBlue->setMinimum(std::min(currentParams.blueBrightness, min.blueBrightness));
        ui->spinBox_colorBrightBlue->setMaximum(std::max(currentParams.blueBrightness, max.blueBrightness));
        ui->horizontalScrollBar_colorBrightBlue->setMinimum(ui->spinBox_colorBrightBlue->minimum());
        ui->horizontalScrollBar_colorBrightBlue->setMaximum(ui->spinBox_colorBrightBlue->maximum());

        ui->spinBox_colorContrBlue->setMinimum(std::min(currentParams.blueContrast, min.blueContrast));
        ui->spinBox_colorContrBlue->setMaximum(std::max(currentParams.blueContrast, max.blueContrast));
        ui->horizontalScrollBar_colorContrBlue->setMinimum(ui->spinBox_colorContrBlue->minimum());
        ui->horizontalScrollBar_colorContrBlue->setMaximum(ui->spinBox_colorContrBlue->maximum());

        ui->spinBox_videoBlackLevel->setMinimum(std::min(currentParams.blackLevel, min.blackLevel));
        ui->spinBox_videoBlackLevel->setMaximum(std::max(currentParams.blackLevel, max.blackLevel));
        ui->horizontalScrollBar_videoBlackLevel->setMinimum(ui->spinBox_videoBlackLevel->minimum());
        ui->horizontalScrollBar_videoBlackLevel->setMaximum(ui->spinBox_videoBlackLevel->maximum());

        ui->spinBox_videoHorPos->setMinimum(std::min(currentParams.horizontalPosition, min.horizontalPosition));
        ui->spinBox_videoHorPos->setMaximum(std::max(currentParams.horizontalPosition, max.horizontalPosition));
        ui->horizontalScrollBar_videoHorPos->setMinimum(ui->spinBox_videoHorPos->minimum());
        ui->horizontalScrollBar_videoHorPos->setMaximum(ui->spinBox_videoHorPos->maximum());

        ui->spinBox_videoHorSize->setMinimum(std::min(currentParams.horizontalScale, min.horizontalScale));
        ui->spinBox_videoHorSize->setMaximum(std::max(currentParams.horizontalScale, max.horizontalScale));
        ui->horizontalScrollBar_videoHorSize->setMinimum(ui->spinBox_videoHorSize->minimum());
        ui->horizontalScrollBar_videoHorSize->setMaximum(ui->spinBox_videoHorSize->maximum());

        ui->spinBox_videoPhase->setMinimum(std::min(currentParams.phase, min.phase));
        ui->spinBox_videoPhase->setMaximum(std::max(currentParams.phase, max.phase));
        ui->horizontalScrollBar_videoPhase->setMinimum(ui->spinBox_videoPhase->minimum());
        ui->horizontalScrollBar_videoPhase->setMaximum(ui->spinBox_videoPhase->maximum());

        ui->spinBox_videoVerPos->setMinimum(std::min(currentParams.verticalPosition, min.verticalPosition));
        ui->spinBox_videoVerPos->setMaximum(std::max(currentParams.verticalPosition, max.verticalPosition));
        ui->horizontalScrollBar_videoVerPos->setMinimum(ui->spinBox_videoVerPos->minimum());
        ui->horizontalScrollBar_videoVerPos->setMaximum(ui->spinBox_videoVerPos->maximum());
    }

    return;
}

// Called to inform the dialog that a new capture signal has been received.
void VideoParameterDialog::notify_of_new_capture_signal(void)
{
    this->update_preset_control_ranges();

    return;
}

void VideoParameterDialog::update_preset_controls(void)
{
    this->currentPreset = kvideopreset_get_preset(ui->comboBox_presetList->currentData().toUInt());

    if (!this->currentPreset)
    {
        return;
    }

    ui->checkBox_activatorResolution->setChecked(this->currentPreset->activatesWithResolution);
    ui->checkBox_activatorRefreshRate->setChecked(this->currentPreset->activatesWithRefreshRate);
    ui->checkBox_activatorShortcut->setChecked(this->currentPreset->activatesWithShortcut);

    ui->doubleSpinBox_refreshRateValue->setValue(this->currentPreset->activationRefreshRate.value<double>());

    // Note: we adjust the min/max values of the spinbox to make sure Qt doesn't
    // modify preset data that falls outside of the existing min/max range.
    ui->spinBox_resolutionX->setMinimum(std::min(ui->spinBox_resolutionX->minimum(), int(this->currentPreset->activationResolution.w)));
    ui->spinBox_resolutionX->setMaximum(std::max(ui->spinBox_resolutionX->maximum(), int(this->currentPreset->activationResolution.w)));
    ui->spinBox_resolutionY->setMinimum(std::min(ui->spinBox_resolutionY->minimum(), int(this->currentPreset->activationResolution.h)));
    ui->spinBox_resolutionY->setMaximum(std::max(ui->spinBox_resolutionY->maximum(), int(this->currentPreset->activationResolution.h)));
    ui->spinBox_resolutionX->setValue(int(this->currentPreset->activationResolution.w));
    ui->spinBox_resolutionY->setValue(int(this->currentPreset->activationResolution.h));

    // Figure out which shortcut this preset is assigned. Expects shortcuts to
    // be strings of the form "ctrl+f1", "ctrl+f2", etc.
    {
        const auto shortcutKeys = QString::fromStdString(this->currentPreset->activationShortcut).toLower().split("+");

        k_assert(((shortcutKeys.length() == 2) &&
                  (shortcutKeys.at(0) == "ctrl")),
                 "Invalid video preset activation shortcut.");

        // Expects the combo box to be populated with 12 items whose text is
        // "F1" to "F12".
        k_assert((ui->comboBox_shortcutSecondKey->count() == 12), "Malformed GUI");
        const int idx = shortcutKeys.at(1).split("f").at(1).toInt();
        ui->comboBox_shortcutSecondKey->setCurrentIndex(idx - 1);
    }

    /// TODO: Check to make sure the correct combo box indices are being set.
    switch (this->currentPreset->refreshRateComparator)
    {
        case video_preset_s::refresh_rate_comparison_e::ceiled:  ui->comboBox_refreshRateComparison->setCurrentIndex(3); break;
        case video_preset_s::refresh_rate_comparison_e::floored: ui->comboBox_refreshRateComparison->setCurrentIndex(2); break;
        case video_preset_s::refresh_rate_comparison_e::rounded: ui->comboBox_refreshRateComparison->setCurrentIndex(1); break;
        case video_preset_s::refresh_rate_comparison_e::equals:  ui->comboBox_refreshRateComparison->setCurrentIndex(0); break;
        default: k_assert(0, "Unknown comparator.");
    }

    ui->lineEdit_presetName->setText(QString::fromStdString(this->currentPreset->name));

    // Assign the video parameter values.
    {
        CONTROLS_LIVE_UPDATE = false;

        this->update_preset_control_ranges();

        // Current values.
        {
            ui->spinBox_colorBright->setValue(this->currentPreset->videoParameters.overallBrightness);
            ui->horizontalScrollBar_colorBright->setValue(this->currentPreset->videoParameters.overallBrightness);

            ui->spinBox_colorContr->setValue(this->currentPreset->videoParameters.overallContrast);
            ui->horizontalScrollBar_colorContr->setValue(this->currentPreset->videoParameters.overallContrast);

            ui->spinBox_colorBrightRed->setValue(this->currentPreset->videoParameters.redBrightness);
            ui->horizontalScrollBar_colorBrightRed->setValue(this->currentPreset->videoParameters.redBrightness);

            ui->spinBox_colorContrRed->setValue(this->currentPreset->videoParameters.redContrast);
            ui->horizontalScrollBar_colorContrRed->setValue(this->currentPreset->videoParameters.redContrast);

            ui->spinBox_colorBrightGreen->setValue(this->currentPreset->videoParameters.greenBrightness);
            ui->horizontalScrollBar_colorBrightGreen->setValue(this->currentPreset->videoParameters.greenBrightness);

            ui->spinBox_colorContrGreen->setValue(this->currentPreset->videoParameters.greenContrast);
            ui->horizontalScrollBar_colorContrGreen->setValue(this->currentPreset->videoParameters.greenContrast);

            ui->spinBox_colorBrightBlue->setValue(this->currentPreset->videoParameters.blueBrightness);
            ui->horizontalScrollBar_colorBrightBlue->setValue(this->currentPreset->videoParameters.blueBrightness);

            ui->spinBox_colorContrBlue->setValue(this->currentPreset->videoParameters.blueContrast);
            ui->horizontalScrollBar_colorContrBlue->setValue(this->currentPreset->videoParameters.blueContrast);

            ui->spinBox_videoBlackLevel->setValue(this->currentPreset->videoParameters.blackLevel);
            ui->horizontalScrollBar_videoBlackLevel->setValue(this->currentPreset->videoParameters.blackLevel);

            ui->spinBox_videoHorPos->setValue(this->currentPreset->videoParameters.horizontalPosition);
            ui->horizontalScrollBar_videoHorPos->setValue(this->currentPreset->videoParameters.horizontalPosition);

            ui->spinBox_videoHorSize->setValue(this->currentPreset->videoParameters.horizontalScale);
            ui->horizontalScrollBar_videoHorSize->setValue(this->currentPreset->videoParameters.horizontalScale);

            ui->spinBox_videoPhase->setValue(this->currentPreset->videoParameters.phase);
            ui->horizontalScrollBar_videoPhase->setValue(this->currentPreset->videoParameters.phase);

            ui->spinBox_videoVerPos->setValue(this->currentPreset->videoParameters.verticalPosition);
            ui->horizontalScrollBar_videoVerPos->setValue(this->currentPreset->videoParameters.verticalPosition);
        }

        CONTROLS_LIVE_UPDATE = true;
    }

    return;
}
