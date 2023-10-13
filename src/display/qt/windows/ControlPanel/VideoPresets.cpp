#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QMenuBar>
#include <QTimer>
#include <QDebug>
#include <functional>
#include <cstring>
#include "display/qt/widgets/VideoPresetList.h"
#include "display/qt/widgets/ParameterGrid.h"
#include "display/qt/widgets/DialogFileMenu.h"
#include "display/qt/windows/ControlPanel/VideoPresets.h"
#include "display/qt/persistent_settings.h"
#include "common/command_line/command_line.h"
#include "common/vcs_event/vcs_event.h"
#include "common/refresh_rate.h"
#include "common/disk/disk.h"
#include "capture/capture.h"
#include "capture/video_presets.h"
#include "ui_VideoPresets.h"

// By default, values from the GUI's controls (sliders and spinboxes) will be
// sent to the capture card in real-time, i.e. as the controls are operated.
// Set this variable to false to disable real-time updates.
static bool CONTROLS_LIVE_UPDATE = true;

#define PROP_LABEL_HORIZONTAL_SIZE     "Horizontal size"
#define PROP_LABEL_HORIZONTAL_POSITION "Horizontal position"
#define PROP_LABEL_VERTICAL_POSITION   "Vertical position"
#define PROP_LABEL_BLACK_LEVEL         "Black level"
#define PROP_LABEL_PHASE               "Phase"
#define PROP_LABEL_BRIGHTNESS          "Brightness"
#define PROP_LABEL_RED_BRIGHTNESS      "Red brightness"
#define PROP_LABEL_GREEN_BRIGHTNESS    "Green brightness"
#define PROP_LABEL_BLUE_BRIGHTNESS     "Blue brightness"
#define PROP_LABEL_CONTRAST            "Contrast"
#define PROP_LABEL_RED_CONTRAST        "Red contrast"
#define PROP_LABEL_GREEN_CONTRAST      "Green contrast"
#define PROP_LABEL_BLUE_CONTRAST       "Blue contrast"

control_panel::VideoPresets::VideoPresets(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::VideoPresets)
{
    ui->setupUi(this);

    this->set_name("Video presets");

    // Set the GUI controls to their proper initial values.
    {
        // The file format we save presets into reserves the { and } characters;
        // these characters shouldn't occur in preset names.
        ui->lineEdit_presetName->setValidator(new QRegExpValidator(QRegExp("[^{}]*"), this));

        ui->groupBox_activation->setEnabled(false);
        ui->comboBox_presetList->setEnabled(false);
        ui->pushButton_deletePreset->setEnabled(false);
        ui->groupBox_videoPresetName->setEnabled(false);
        ui->parameterGrid_properties->setEnabled(false);

        const auto minres = resolution_s::from_capture_device_properties(": minimum");
        const auto maxres = resolution_s::from_capture_device_properties(": maximum");

        ui->spinBox_resolutionX->setMinimum(int(minres.w));
        ui->spinBox_resolutionX->setMaximum(int(maxres.w));
        ui->spinBox_resolutionY->setMinimum(int(minres.w));
        ui->spinBox_resolutionY->setMaximum(int(maxres.w));

        for (const char* propName: kc_supported_video_preset_properties())
        {
            ui->parameterGrid_properties->add_scroller(propName);
        }

        this->update_active_preset_indicator();
        this->update_property_control_ranges(analog_properties_s::from_capture_device_properties(": default"));
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->pushButton_openPresetFile, &QPushButton::clicked, this, [this]
        {
            QString filename = QFileDialog::getOpenFileName(
                this,
                "Load video presets from file",
                "",
                "Video presets (*.vcs-video);;All files(*.*)"
            );

            this->load_presets_from_file(filename);
        });

        connect(ui->pushButton_closePresetFile, &QPushButton::clicked, this, [this]
        {
            if (
                this->has_unsaved_changes() &&
                (
                    QMessageBox::No == QMessageBox::question(
                        nullptr,
                        "Confirm preset reset",
                        "There are unsaved changes in one or more open presets.\n\n"
                        "Are you sure you want to proceed and lose your unsaved data?",
                        (QMessageBox::No | QMessageBox::Yes)
                    )
                )
            ){
                return;
            }

            this->ui->comboBox_presetList->remove_all_presets();
            this->set_data_filename("");
            kvideopreset_remove_all_presets();
        });

        connect(ui->pushButton_savePresetFile, &QPushButton::clicked, this, [this]
        {
            this->save_video_presets_to_file(this->data_filename());
        });

        connect(ui->pushButton_savePresetFileAs, &QPushButton::clicked, this, [this]
        {
            QString filename = QFileDialog::getSaveFileName(
                this,
                "Save video presets to file",
                this->data_filename(),
                "Video presets (*.vcs-video);;All files (*.*)"
            );

            this->save_video_presets_to_file(filename);
        });

        connect(this, &DialogFragment::unsaved_changes_flag_changed, this, [this](const bool areUnsaved)
        {
            this->ui->pushButton_savePresetFile->setEnabled(
                areUnsaved &&
                !this->data_filename().isEmpty() &&
                ui->comboBox_presetList->count()
            );

            this->ui->pushButton_savePresetFileAs->setEnabled(
                this->ui->pushButton_savePresetFileAs->isEnabled() ||
                (
                    areUnsaved &&
                    this->data_filename().isEmpty() &&
                    ui->comboBox_presetList->count()
                )
            );
        });

        connect(this, &DialogFragment::data_filename_changed, this, [this](const QString &newFilename)
        {
            kpers_set_value(INI_GROUP_VIDEO_PRESETS, "SourceFile", newFilename);
            this->ui->lineEdit_presetFilename->setText(QFileInfo(newFilename).fileName());
            this->ui->lineEdit_presetFilename->setToolTip(newFilename);

            this->ui->pushButton_savePresetFileAs->setDisabled(newFilename.isEmpty());
            this->ui->pushButton_closePresetFile->setDisabled(newFilename.isEmpty());
        });

        connect(this, &VideoPresets::preset_activation_rules_changed, this, [this]
        {
            if (CONTROLS_LIVE_UPDATE)
            {
                kvideopreset_apply_current_active_preset();
            }
        });

        connect(ui->comboBox_presetList, &VideoPresetList::preset_selected, this, [this]
        {
            CONTROLS_LIVE_UPDATE = false;
            this->lock_unsaved_changes_flag(true);
            this->update_preset_controls_with_current_preset_data();
            this->lock_unsaved_changes_flag(false);
            CONTROLS_LIVE_UPDATE = true;

            this->update_active_preset_indicator();
        });

        connect(ui->comboBox_presetList, &VideoPresetList::list_became_empty, this, [this]
        {
            ui->pushButton_savePresetFileAs->setEnabled(false);
            ui->groupBox_activation->setEnabled(false);
            ui->comboBox_presetList->setEnabled(false);
            ui->pushButton_deletePreset->setEnabled(false);
            ui->groupBox_videoPresetName->setEnabled(false);
            ui->parameterGrid_properties->setEnabled(false);
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
            ui->parameterGrid_properties->setEnabled(true);
            ui->pushButton_deletePreset->setEnabled(true);
        });

        connect(ui->comboBox_presetList, &VideoPresetList::preset_label_changed, this, [this]
        {
            this->ui->comboBox_presetList->sort_alphabetically();
        });

        connect(ui->pushButton_deletePreset, &QPushButton::clicked, this, [this]
        {
            auto *const selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset && (QMessageBox::question(this, "Confirm preset deletion", "Are you sure you want to delete this preset?", (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes))
            {
                const bool wasActivePreset = kvideopreset_is_preset_active(selectedPreset);

                ui->comboBox_presetList->remove_preset(selectedPreset->id);
                kvideopreset_remove_preset(selectedPreset->id);
                emit this->data_changed();

                if (wasActivePreset)
                {
                    kvideopreset_apply_current_active_preset();
                }
            }
        });

        connect(ui->pushButton_addNewPreset, &QPushButton::clicked, this, [this]
        {
            const bool duplicateCurrent = (qApp->keyboardModifiers() & Qt::AltModifier);
            analog_video_preset_s *const newPreset = kvideopreset_create_new_preset(duplicateCurrent? ui->comboBox_presetList->current_preset() : nullptr);

            if (!duplicateCurrent)
            {
                newPreset->activationResolution = resolution_s::from_capture_device_properties();
                newPreset->activationRefreshRate = refresh_rate_s::from_capture_device_properties();
            }

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

                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });

        connect(ui->parameterGrid_properties, &ParameterGrid::parameter_value_changed_by_user, this, [this]
        {
            emit this->data_changed();
        });

        connect(ui->parameterGrid_properties, &ParameterGrid::parameter_value_changed, this, [this]
        {
            this->broadcast_current_preset_parameters();
        });

        connect(ui->lineEdit_presetName, &QLineEdit::textEdited, this, [this](const QString &text)
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->name = text.toStdString();
                ev_video_preset_name_changed.fire(selectedPreset);

                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->checkBox_activatorRefreshRate, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->doubleSpinBox_refreshRateValue->setEnabled(checked);
            ui->comboBox_refreshRateComparison->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithRefreshRate = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }

            emit this->preset_activation_rules_changed(selectedPreset);
        });

        connect(ui->checkBox_activatorResolution, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->spinBox_resolutionX->setEnabled(checked);
            ui->spinBox_resolutionY->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithResolution = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }

            emit this->preset_activation_rules_changed(selectedPreset);
        });

        connect(ui->checkBox_activatorShortcut, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->comboBox_shortcutKey->setEnabled(checked);

            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                selectedPreset->activatesWithShortcut = checked;
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->comboBox_shortcutKey, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]
        {
            auto *selectedPreset = ui->comboBox_presetList->current_preset();

            if (selectedPreset)
            {
                ui->comboBox_presetList->current_preset()->activationShortcut =  ui->comboBox_shortcutKey->currentText().toStdString();
                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);
                emit this->data_changed();
            }
        });

        connect(ui->comboBox_refreshRateComparison, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int idx)
        {
            const QString text = ui->comboBox_refreshRateComparison->itemText(idx);

            if (text == "Exactly")
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
                if (text == "Exactly")      selectedPreset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::equals;
                else if (text == "Rounded") selectedPreset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::rounded;
                else if (text == "Floored") selectedPreset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::floored;
                else if (text == "Ceiled")  selectedPreset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::ceiled;

                ui->comboBox_presetList->update_preset_item_label(selectedPreset->id);

                emit this->preset_activation_rules_changed(selectedPreset);
                emit this->data_changed();
            }
        });
    }

    // Restore persistent settings.
    {
        if (kcom_video_presets_file_name().empty())
        {
            const QString presetsSourceFile = kpers_value_of(INI_GROUP_VIDEO_PRESETS, "SourceFile", QString()).toString();
            kcom_override_video_presets_file_name(presetsSourceFile.toStdString());
        }
    }

    // Register app event listeners.
    {
        ev_video_preset_activated.listen([this]
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

control_panel::VideoPresets::~VideoPresets()
{
    delete ui;

    return;
}

bool control_panel::VideoPresets::save_video_presets_to_file(QString filename)
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

bool control_panel::VideoPresets::load_presets_from_file(const QString &filename)
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

void control_panel::VideoPresets::update_active_preset_indicator(void)
{
    const auto *selectedPreset = ui->comboBox_presetList->current_preset();

    if (ui->comboBox_presetList->count() <= 0)
    {
        ui->label_isPresetCurrentlyActive->setProperty("presetStatus", "disabled");
        ui->label_isPresetCurrentlyActive->setToolTip("");
    }
    else if (kvideopreset_is_preset_active(selectedPreset))
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

void control_panel::VideoPresets::assign_presets(const std::vector<analog_video_preset_s*> &presets)
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

void control_panel::VideoPresets::broadcast_current_preset_parameters(void)
{
    auto *const preset = ui->comboBox_presetList->current_preset();

    if (!preset || !CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    for (const std::string propName: kc_supported_video_preset_properties())
    {
        preset->properties.at(propName) = ui->parameterGrid_properties->value(QString::fromStdString(propName));
    }

    kc_ev_video_preset_params_changed.fire(preset);

    return;
}

void control_panel::VideoPresets::update_property_control_ranges(
    const properties_map_t &properties
)
{
    for (const std::string &propName: kc_supported_video_preset_properties())
    {
        const auto minVal = kc_device_property(propName + ": minimum");
        const auto maxVal = kc_device_property(propName + ": maximum");
        ui->parameterGrid_properties->set_minimum_value(QString::fromStdString(propName), std::min(properties.at(propName), minVal));
        ui->parameterGrid_properties->set_maximum_value(QString::fromStdString(propName), std::max(properties.at(propName), maxVal));
    }

    return;
}

void control_panel::VideoPresets::update_preset_controls_with_current_preset_data(void)
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
    ui->spinBox_resolutionX->setValue(preset->activationResolution.w);
    ui->spinBox_resolutionY->setValue(preset->activationResolution.h);

    // Figure out which shortcut this preset is assigned. Expects shortcuts to
    // be strings of the form "f1", "f2", etc.
    {
        const QString shortcut = QString::fromStdString(preset->activationShortcut);

        // Expects the combo box to be populated with 12 items whose text is
        // "F1" to "F12".
        k_assert((ui->comboBox_shortcutKey->count() == 12), "Malformed GUI");
        const int idx = shortcut.split("F").at(1).toInt();
        ui->comboBox_shortcutKey->setCurrentIndex(idx - 1);
    }

    /// TODO: Check to make sure the correct combo box indices are being set.
    switch (preset->refreshRateComparator)
    {
        case analog_video_preset_s::refresh_rate_comparison_e::ceiled:  ui->comboBox_refreshRateComparison->setCurrentIndex(3); break;
        case analog_video_preset_s::refresh_rate_comparison_e::floored: ui->comboBox_refreshRateComparison->setCurrentIndex(2); break;
        case analog_video_preset_s::refresh_rate_comparison_e::rounded: ui->comboBox_refreshRateComparison->setCurrentIndex(1); break;
        case analog_video_preset_s::refresh_rate_comparison_e::equals:  ui->comboBox_refreshRateComparison->setCurrentIndex(0); break;
        default: k_assert(0, "Unknown comparator.");
    }

    ui->lineEdit_presetName->setText(QString::fromStdString(preset->name));

    // Assign the video parameter values.
    {
        this->update_property_control_ranges(preset->properties);

        for (const auto propName: kc_supported_video_preset_properties())
        {
            ui->parameterGrid_properties->set_value(QString::fromStdString(propName), preset->properties.at(propName));
        }
    }

    return;
}
