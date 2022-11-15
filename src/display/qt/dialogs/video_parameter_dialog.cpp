#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include <cstring>
#include "display/qt/subclasses/QComboBox_video_preset_list.h"
#include "display/qt/subclasses/QGroupBox_parameter_grid.h"
#include "display/qt/subclasses/QMenu_dialog_file_menu.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/command_line/command_line.h"
#include "common/propagate/vcs_event.h"
#include "common/refresh_rate.h"
#include "common/disk/disk.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "capture/video_presets.h"
#include "ui_video_parameter_dialog.h"

// By default, values from the GUI's controls (sliders and spinboxes) will be
// sent to the capture card in real-time, i.e. as the controls are operated.
// Set this variable to false to disable real-time updates.
static bool CONTROLS_LIVE_UPDATE = true;

VideoParameterDialog::VideoParameterDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::VideoParameterDialog)
{
    ui->setupUi(this);

    this->set_name("Video presets");

    // Create the dialog's menu bar.
    {
        this->menuBar = new QMenuBar(this);

        // File...
        {
            auto *const file = new DialogFileMenu(this);

            this->menuBar->addMenu(file);

            connect(file, &DialogFileMenu::save, this, [=](const QString &filename)
            {
                this->save_video_presets_to_file(filename);
            });

            connect(file, &DialogFileMenu::open, this, [=]
            {
                QString filename = QFileDialog::getOpenFileName(this,
                                                                "Load video presets from...",
                                                                "",
                                                                "Video presets (*.vcs-video);;All files(*.*)");

                this->load_presets_from_file(filename);
            });

            connect(file, &DialogFileMenu::save_as, this, [=](const QString &originalFilename)
            {
                QString filename = QFileDialog::getSaveFileName(this,
                                                                "Save video presets as...",
                                                                originalFilename,
                                                                "Video presets (*.vcs-video);;All files (*.*)");

                this->save_video_presets_to_file(filename);
            });

            connect(file, &DialogFileMenu::close, this, [=]
            {
                ui->comboBox_presetList->remove_all_presets();
                this->set_data_filename("");
                kvideopreset_remove_all_presets();
            });
        }

        this->layout()->setMenuBar(this->menuBar);
    }

    // Set the GUI controls to their proper initial values.
    {
        // The file format we save presets into reserves the { and } characters;
        // these characters shouldn't occur in preset names.
        ui->lineEdit_presetName->setValidator(new QRegExpValidator(QRegExp("[^{}]*"), this));

        ui->pushButton_resolutionSeparator->setText("\u00d7");

        ui->groupBox_activation->setEnabled(false);
        ui->comboBox_presetList->setEnabled(false);
        ui->pushButton_deletePreset->setEnabled(false);
        ui->groupBox_videoPresetName->setEnabled(false);
        ui->parameterGrid_videoParams->setEnabled(false);

        const auto minres = kc_get_device_minimum_resolution();
        const auto maxres = kc_get_device_maximum_resolution();

        ui->spinBox_resolutionX->setMinimum(int(minres.w));
        ui->spinBox_resolutionX->setMaximum(int(maxres.w));
        ui->spinBox_resolutionY->setMinimum(int(minres.w));
        ui->spinBox_resolutionY->setMaximum(int(maxres.w));

        ui->parameterGrid_videoParams->add_scroller("Hor. size");
        ui->parameterGrid_videoParams->add_scroller("Hor. position");
        ui->parameterGrid_videoParams->add_scroller("Ver. position");
        ui->parameterGrid_videoParams->add_scroller("Black level");
        ui->parameterGrid_videoParams->add_scroller("Phase");
        ui->parameterGrid_videoParams->add_separator();
        ui->parameterGrid_videoParams->add_scroller("Brightness");
        ui->parameterGrid_videoParams->add_scroller("Red br.");
        ui->parameterGrid_videoParams->add_scroller("Green br.");
        ui->parameterGrid_videoParams->add_scroller("Blue br.");
        ui->parameterGrid_videoParams->add_separator();
        ui->parameterGrid_videoParams->add_scroller("Contrast");
        ui->parameterGrid_videoParams->add_scroller("Red ct.");
        ui->parameterGrid_videoParams->add_scroller("Green ct.");
        ui->parameterGrid_videoParams->add_scroller("Blue ct.");

        this->update_active_preset_indicator();
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(this, &VideoParameterDialog::preset_activation_rules_changed, this, [this]
        {
            this->update_active_preset_indicator();
        });

        connect(this, &VCSBaseDialog::data_filename_changed, this, [this](const QString &newFilename)
        {
            kpers_set_value(INI_GROUP_VIDEO_PRESETS, "presets_source_file", newFilename);
        });

        connect(ui->comboBox_presetList, &VideoPresetList::preset_selected, this, [this]
        {
            this->lock_unsaved_changes_flag(true);
            this->update_preset_controls_with_current_preset_data();
            this->update_active_preset_indicator();
            this->lock_unsaved_changes_flag(false);

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->comboBox_presetList, &VideoPresetList::list_became_empty, this, [this]
        {
            ui->groupBox_activation->setEnabled(false);
            ui->comboBox_presetList->setEnabled(false);
            ui->pushButton_deletePreset->setEnabled(false);
            ui->groupBox_videoPresetName->setEnabled(false);
            ui->parameterGrid_videoParams->setEnabled(false);
            ui->pushButton_deletePreset->setEnabled(false);
            ui->lineEdit_presetName->clear();
            this->update_active_preset_indicator();
        });

        connect(ui->comboBox_presetList, &VideoPresetList::list_became_populated, this, [this]
        {
            ui->groupBox_activation->setEnabled(true);
            ui->comboBox_presetList->setEnabled(true);
            ui->pushButton_deletePreset->setEnabled(true);
            ui->groupBox_videoPresetName->setEnabled(true);
            ui->parameterGrid_videoParams->setEnabled(true);
            ui->pushButton_deletePreset->setEnabled(true);
        });

        connect(ui->pushButton_deletePreset, &QPushButton::clicked, this, [this]
        {
            auto *const selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset &&
                (QMessageBox::question(this, "Confirm deletion",
                                       "Delete this preset?",
                                       (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes))
            {

                ui->comboBox_presetList->remove_preset(selectedPreset->id);
                kvideopreset_remove_preset(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->pushButton_addNewPreset, &QPushButton::clicked, this, [this]
        {
            const bool duplicateCurrent = (QGuiApplication::keyboardModifiers() & Qt::AltModifier);
            video_preset_s *const newPreset = kvideopreset_create_new_preset(duplicateCurrent? ui->comboBox_presetList->current_preset() : nullptr);

            ui->comboBox_presetList->add_preset(newPreset->id);
            emit this->data_changed();
        });

        connect(ui->doubleSpinBox_refreshRateValue, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](const double value)
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activationRefreshRate = value;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);

                kvideopreset_apply_current_active_preset();
                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });

        connect(ui->spinBox_resolutionX, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value)
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activationResolution.w = std::min(MAX_CAPTURE_WIDTH, unsigned(value));
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);

                kvideopreset_apply_current_active_preset();
                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });

        connect(ui->spinBox_resolutionY, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value)
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activationResolution.h = std::min(MAX_CAPTURE_HEIGHT, unsigned(value));
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);

                kvideopreset_apply_current_active_preset();
                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });

        connect(ui->parameterGrid_videoParams, &ParameterGrid::parameter_value_changed_by_user, this, [this]
        {
            emit this->data_changed();
        });

        connect(ui->parameterGrid_videoParams, &ParameterGrid::parameter_value_changed, this, [this]
        {
            this->broadcast_current_preset_parameters();
        });

        connect(ui->lineEdit_presetName, &QLineEdit::textEdited, this, [this](const QString &text)
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->name = text.toStdString();
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->checkBox_activatorRefreshRate, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->doubleSpinBox_refreshRateValue->setEnabled(checked);
            ui->comboBox_refreshRateComparison->setEnabled(checked);
            ui->pushButton_refreshRateSeparator->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithRefreshRate = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }

            kvideopreset_apply_current_active_preset();
            emit this->preset_activation_rules_changed(selectedPreset);
        });

        connect(ui->checkBox_activatorResolution, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->spinBox_resolutionX->setEnabled(checked);
            ui->spinBox_resolutionY->setEnabled(checked);
            ui->pushButton_resolutionSeparator->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithResolution = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }

            kvideopreset_apply_current_active_preset();
            emit this->preset_activation_rules_changed(selectedPreset);
        });

        connect(ui->checkBox_activatorShortcut, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->comboBox_shortcutFirstKey->setEnabled(checked);
            ui->comboBox_shortcutSecondKey->setEnabled(checked);
            ui->label_shortcutSeparator->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithShortcut = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->pushButton_resolutionSeparator, &QPushButton::clicked, this, [this](void)
        {
            const auto currentResolution = kc_get_capture_resolution();

            ui->spinBox_resolutionX->setValue(int(currentResolution.w));
            ui->spinBox_resolutionY->setValue(int(currentResolution.h));
        });

        connect(ui->pushButton_refreshRateSeparator, &QPushButton::clicked, this, [this](void)
        {
            ui->doubleSpinBox_refreshRateValue->setValue(kc_get_capture_refresh_rate().value<double>());
        });

        connect(ui->comboBox_shortcutSecondKey, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int idx)
        {
            const QString text = ui->comboBox_shortcutSecondKey->itemText(idx);
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                QString newShortcut = QString("Ctrl+%1").arg(text);
                ui->comboBox_presetList->current_preset()->activationShortcut = newShortcut.toStdString();
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->comboBox_refreshRateComparison, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int idx)
        {
            const QString text = ui->comboBox_refreshRateComparison->itemText(idx);

            if (text == "Exact")
            {
                ui->doubleSpinBox_refreshRateValue->setDecimals(refresh_rate_s::numDecimalsPrecision);
            }
            else
            {
                ui->doubleSpinBox_refreshRateValue->setDecimals(0);
            }

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                if (text == "Exact")        selectedPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::equals;
                else if (text == "Rounded") selectedPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::rounded;
                else if (text == "Floored") selectedPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::floored;
                else if (text == "Ceiled")  selectedPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::ceiled;

                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);

                kvideopreset_apply_current_active_preset();
                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "video_parameters", this->size()).toSize());

        if (kcom_video_presets_file_name().empty())
        {
            const QString presetsSourceFile = kpers_value_of(INI_GROUP_VIDEO_PRESETS, "presets_source_file", QString()).toString();
            kcom_override_video_presets_file_name(presetsSourceFile.toStdString());
        }
    }

    // Register app event listeners.
    {
        kc_evNewVideoMode.listen([this](const video_mode_s&)
        {
            if (kc_is_receiving_signal())
            {
                this->update_preset_control_ranges();
            }

            this->update_active_preset_indicator();
        });

        kc_evSignalLost.listen([this]
        {
            this->update_active_preset_indicator();
        });
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

bool VideoParameterDialog::save_video_presets_to_file(QString filename)
{
    if (filename.isEmpty())
    {
        return false;
    }

    if (!QStringList({"vcs-video"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-video");
    }

    if (kdisk_save_video_presets(kvideopreset_all_presets(), filename.toStdString()))
    {
        this->set_data_filename(filename);

        return true;
    }

    return false;
}

bool VideoParameterDialog::load_presets_from_file(const QString &filename)
{
    if (filename.isEmpty())
    {
        return false;
    }

    const auto presets = kdisk_load_video_presets(filename.toStdString());

    this->set_data_filename(filename);

    if (!presets.empty())
    {
        kvideopreset_assign_presets(presets);
        this->assign_presets(presets);
        this->set_unsaved_changes_flag(false);
    }

    return true;
}

void VideoParameterDialog::update_active_preset_indicator(void)
{
    const auto *selectedPreset = ui->comboBox_presetList->current_preset();

    if (ui->comboBox_presetList->count() <= 0)
    {
        ui->label_isPresetCurrentlyActive->setProperty("presetStatus", "disabled");
        ui->label_isPresetCurrentlyActive->setToolTip("");
    }
    else if (selectedPreset && kvideopreset_is_preset_active(selectedPreset))
    {
        ui->label_isPresetCurrentlyActive->setProperty("presetStatus", "active");
        ui->label_isPresetCurrentlyActive->setToolTip("This preset is active");
    }
    else
    {
        ui->label_isPresetCurrentlyActive->setProperty("presetStatus", "inactive");
        ui->label_isPresetCurrentlyActive->setToolTip("This preset is inactive");
    }

    this->style()->polish(ui->label_isPresetCurrentlyActive);

    return;
}

void VideoParameterDialog::assign_presets(const std::vector<video_preset_s*> &presets)
{
    ui->comboBox_presetList->clear();

    for (auto *const preset: presets)
    {
        ui->comboBox_presetList->add_preset(preset->id, true);
    }

    /// TODO: It would be better to sort the items by (numeric) resolution.
    ui->comboBox_presetList->sort_alphabetically();

    ui->comboBox_presetList->setCurrentIndex(0);
    emit ui->comboBox_presetList->preset_selected(0); // Manually force the signal, in case there's only one preset in the list.

    return;
}

void VideoParameterDialog::broadcast_current_preset_parameters(void)
{
    auto *const preset = ui->comboBox_presetList->current_preset();

    if (!preset ||
        !CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    preset->videoParameters.overallBrightness  = ui->parameterGrid_videoParams->value("Brightness");
    preset->videoParameters.overallContrast    = ui->parameterGrid_videoParams->value("Contrast");
    preset->videoParameters.redBrightness      = ui->parameterGrid_videoParams->value("Red br.");
    preset->videoParameters.redContrast        = ui->parameterGrid_videoParams->value("Red ct.");
    preset->videoParameters.greenBrightness    = ui->parameterGrid_videoParams->value("Green br.");
    preset->videoParameters.greenContrast      = ui->parameterGrid_videoParams->value("Green ct.");
    preset->videoParameters.blueBrightness     = ui->parameterGrid_videoParams->value("Blue br.");
    preset->videoParameters.blueContrast       = ui->parameterGrid_videoParams->value("Blue ct.");
    preset->videoParameters.blackLevel         = ui->parameterGrid_videoParams->value("Black level");
    preset->videoParameters.horizontalPosition = ui->parameterGrid_videoParams->value("Hor. position");
    preset->videoParameters.horizontalScale    = ui->parameterGrid_videoParams->value("Hor. size");
    preset->videoParameters.phase              = ui->parameterGrid_videoParams->value("Phase");
    preset->videoParameters.verticalPosition   = ui->parameterGrid_videoParams->value("Ver. position");

    kc_evVideoPresetParamsChanged.fire(preset);

    return;
}

void VideoParameterDialog::update_preset_control_ranges(void)
{
    video_signal_parameters_s currentParams = {};

    if (ui->comboBox_presetList->current_preset())
    {
        currentParams = ui->comboBox_presetList->current_preset()->videoParameters;
    }

    // Current valid ranges. Note: we adjust the min/max values so that Qt
    // doesn't clip preset data that falls outside of the capture card's
    // min/max range.
    {
        const video_signal_parameters_s min = kc_get_device_video_parameter_minimums();
        const video_signal_parameters_s max = kc_get_device_video_parameter_maximums();
        const video_signal_parameters_s def = kc_get_device_video_parameter_defaults();

        ui->parameterGrid_videoParams->set_maximum_value("Hor. size", std::max(currentParams.horizontalScale, max.horizontalScale));
        ui->parameterGrid_videoParams->set_maximum_value("Hor. position", std::max(currentParams.horizontalPosition, max.horizontalPosition));
        ui->parameterGrid_videoParams->set_maximum_value("Ver. position", std::max(currentParams.verticalPosition, max.verticalPosition));
        ui->parameterGrid_videoParams->set_maximum_value("Black level", std::max(currentParams.blackLevel, max.blackLevel));
        ui->parameterGrid_videoParams->set_maximum_value("Phase", std::max(currentParams.phase, max.phase));
        ui->parameterGrid_videoParams->set_maximum_value("Brightness", std::max(currentParams.overallBrightness, max.overallBrightness));
        ui->parameterGrid_videoParams->set_maximum_value("Red br.", std::max(currentParams.redBrightness, max.redBrightness));
        ui->parameterGrid_videoParams->set_maximum_value("Green br.", std::max(currentParams.greenBrightness, max.greenBrightness));
        ui->parameterGrid_videoParams->set_maximum_value("Blue br.", std::max(currentParams.blueBrightness, max.blueBrightness));
        ui->parameterGrid_videoParams->set_maximum_value("Contrast", std::max(currentParams.overallContrast, max.overallContrast));
        ui->parameterGrid_videoParams->set_maximum_value("Red ct.", std::max(currentParams.redContrast, max.redContrast));
        ui->parameterGrid_videoParams->set_maximum_value("Green ct.", std::max(currentParams.greenContrast, max.greenContrast));
        ui->parameterGrid_videoParams->set_maximum_value("Blue ct.", std::max(currentParams.blueContrast, max.blueContrast));

        ui->parameterGrid_videoParams->set_minimum_value("Hor. size", std::min(currentParams.horizontalScale, min.horizontalScale));
        ui->parameterGrid_videoParams->set_minimum_value("Hor. position", std::min(currentParams.horizontalPosition, min.horizontalPosition));
        ui->parameterGrid_videoParams->set_minimum_value("Ver. position", std::min(currentParams.verticalPosition, min.verticalPosition));
        ui->parameterGrid_videoParams->set_minimum_value("Black level", std::min(currentParams.blackLevel, min.blackLevel));
        ui->parameterGrid_videoParams->set_minimum_value("Phase", std::min(currentParams.phase, min.phase));
        ui->parameterGrid_videoParams->set_minimum_value("Brightness", std::min(currentParams.overallBrightness, min.overallBrightness));
        ui->parameterGrid_videoParams->set_minimum_value("Red br.", std::min(currentParams.redBrightness, min.redBrightness));
        ui->parameterGrid_videoParams->set_minimum_value("Green br.", std::min(currentParams.greenBrightness, min.greenBrightness));
        ui->parameterGrid_videoParams->set_minimum_value("Blue br.", std::min(currentParams.blueBrightness, min.blueBrightness));
        ui->parameterGrid_videoParams->set_minimum_value("Contrast", std::min(currentParams.overallContrast, min.overallContrast));
        ui->parameterGrid_videoParams->set_minimum_value("Red ct.", std::min(currentParams.redContrast, min.redContrast));
        ui->parameterGrid_videoParams->set_minimum_value("Green ct.",std::min(currentParams.greenContrast,  min.greenContrast));
        ui->parameterGrid_videoParams->set_minimum_value("Blue ct.", std::min(currentParams.blueContrast, min.blueContrast));

        ui->parameterGrid_videoParams->set_default_value("Hor. size", def.horizontalScale);
        ui->parameterGrid_videoParams->set_default_value("Hor. position", def.horizontalPosition);
        ui->parameterGrid_videoParams->set_default_value("Ver. position", def.verticalPosition);
        ui->parameterGrid_videoParams->set_default_value("Black level", def.blackLevel);
        ui->parameterGrid_videoParams->set_default_value("Phase", def.phase);
        ui->parameterGrid_videoParams->set_default_value("Brightness", def.overallBrightness);
        ui->parameterGrid_videoParams->set_default_value("Red br.", def.redBrightness);
        ui->parameterGrid_videoParams->set_default_value("Green br.", def.greenBrightness);
        ui->parameterGrid_videoParams->set_default_value("Blue br.", def.blueBrightness);
        ui->parameterGrid_videoParams->set_default_value("Contrast", def.overallContrast);
        ui->parameterGrid_videoParams->set_default_value("Red ct.", def.redContrast);
        ui->parameterGrid_videoParams->set_default_value("Green ct.", def.greenContrast);
        ui->parameterGrid_videoParams->set_default_value("Blue ct.", def.blueContrast);
    }

    return;
}

void VideoParameterDialog::update_preset_controls_with_current_preset_data(void)
{
    auto *const preset = ui->comboBox_presetList->current_preset();

    if (!preset)
    {
        return;
    }

    ui->checkBox_activatorResolution->setChecked(preset->activatesWithResolution);
    ui->checkBox_activatorRefreshRate->setChecked(preset->activatesWithRefreshRate);
    ui->checkBox_activatorShortcut->setChecked(preset->activatesWithShortcut);

    ui->doubleSpinBox_refreshRateValue->setValue(preset->activationRefreshRate.value<double>());

    // Note: we adjust the min/max values of the spinbox to make sure Qt doesn't
    // modify preset data that falls outside of the existing min/max range.
    ui->spinBox_resolutionX->setMinimum(std::min(ui->spinBox_resolutionX->minimum(), int(preset->activationResolution.w)));
    ui->spinBox_resolutionX->setMaximum(std::max(ui->spinBox_resolutionX->maximum(), int(preset->activationResolution.w)));
    ui->spinBox_resolutionY->setMinimum(std::min(ui->spinBox_resolutionY->minimum(), int(preset->activationResolution.h)));
    ui->spinBox_resolutionY->setMaximum(std::max(ui->spinBox_resolutionY->maximum(), int(preset->activationResolution.h)));
    ui->spinBox_resolutionX->setValue(int(preset->activationResolution.w));
    ui->spinBox_resolutionY->setValue(int(preset->activationResolution.h));

    // Figure out which shortcut this preset is assigned. Expects shortcuts to
    // be strings of the form "ctrl+f1", "ctrl+f2", etc.
    {
        const auto shortcutKeys = QString::fromStdString(preset->activationShortcut).toLower().split("+");

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
    switch (preset->refreshRateComparator)
    {
        case video_preset_s::refresh_rate_comparison_e::ceiled:  ui->comboBox_refreshRateComparison->setCurrentIndex(3); break;
        case video_preset_s::refresh_rate_comparison_e::floored: ui->comboBox_refreshRateComparison->setCurrentIndex(2); break;
        case video_preset_s::refresh_rate_comparison_e::rounded: ui->comboBox_refreshRateComparison->setCurrentIndex(1); break;
        case video_preset_s::refresh_rate_comparison_e::equals:  ui->comboBox_refreshRateComparison->setCurrentIndex(0); break;
        default: k_assert(0, "Unknown comparator.");
    }

    ui->lineEdit_presetName->setText(QString::fromStdString(preset->name));

    // Assign the video parameter values.
    {
        CONTROLS_LIVE_UPDATE = false;

        this->update_preset_control_ranges();

        // Current values.
        {
            const auto &currentParams = preset->videoParameters;

            ui->parameterGrid_videoParams->set_value("Hor. size", currentParams.horizontalScale);
            ui->parameterGrid_videoParams->set_value("Hor. position", currentParams.horizontalPosition);
            ui->parameterGrid_videoParams->set_value("Ver. position", currentParams.verticalPosition);
            ui->parameterGrid_videoParams->set_value("Black level", currentParams.blackLevel);
            ui->parameterGrid_videoParams->set_value("Phase", currentParams.phase);
            ui->parameterGrid_videoParams->set_value("Brightness", currentParams.overallBrightness);
            ui->parameterGrid_videoParams->set_value("Red br.", currentParams.redBrightness);
            ui->parameterGrid_videoParams->set_value("Green br.", currentParams.greenBrightness);
            ui->parameterGrid_videoParams->set_value("Blue br.", currentParams.blueBrightness);
            ui->parameterGrid_videoParams->set_value("Contrast", currentParams.overallContrast);
            ui->parameterGrid_videoParams->set_value("Red ct.", currentParams.redContrast);
            ui->parameterGrid_videoParams->set_value("Green ct.", currentParams.greenContrast);
            ui->parameterGrid_videoParams->set_value("Blue ct.", currentParams.blueContrast);
        }

        CONTROLS_LIVE_UPDATE = true;
    }

    return;
}
