#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include <cstring>
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
#include "display/qt/subclasses/QGroupBox_parameter_grid.h"
#include "display/qt/subclasses/QMenu_dialog_file_menu.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/command_line/command_line.h"
#include "common/propagate/app_events.h"
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
                this->remove_all_video_presets_from_list();
                this->set_data_filename("");
                kvideopreset_remove_all_presets();
                kvideopreset_apply_current_active_preset();
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

        const auto minres = kc_capture_api().get_minimum_resolution();
        const auto maxres = kc_capture_api().get_maximum_resolution();

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
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(this, &VCSBaseDialog::data_filename_changed, this, [this](const QString &newFilename)
        {
            this->set_unsaved_changes(false);
            kpers_set_value(INI_GROUP_VIDEO_PRESETS, "presets_source_file", newFilename);
        });

        connect(this, &VideoParameterDialog::preset_list_became_empty, this, [this]
        {
            ui->groupBox_activation->setEnabled(false);
            ui->comboBox_presetList->setEnabled(false);
            ui->pushButton_deletePreset->setEnabled(false);
            ui->groupBox_videoPresetName->setEnabled(false);
            ui->parameterGrid_videoParams->setEnabled(false);
            ui->pushButton_deletePreset->setEnabled(false);
            ui->lineEdit_presetName->clear();
        });

        connect(this, &VideoParameterDialog::preset_list_no_longer_empty, this, [this]
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
            if (this->currentPreset &&
                (QMessageBox::question(this, "Confirm deletion",
                                       "Delete this preset?",
                                       (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes))
            {
                const auto presetToDelete = this->currentPreset;
                this->remove_video_preset_from_list(presetToDelete);
                kvideopreset_remove_preset(presetToDelete->id);
                kvideopreset_apply_current_active_preset();
            }
        });

        connect(ui->pushButton_addNewPreset, &QPushButton::clicked, this, [this]
        {
            const bool duplicateCurrent = (QGuiApplication::keyboardModifiers() & Qt::AltModifier);
            video_preset_s *const newPreset = kvideopreset_create_new_preset(duplicateCurrent? this->currentPreset : nullptr);

            this->add_video_preset_to_list(newPreset);
            emit this->data_changed();
        });

        #define OVERLOAD_DOUBLE static_cast<void (QDoubleSpinBox::*)(double)>

        connect(ui->doubleSpinBox_refreshRateValue, OVERLOAD_DOUBLE(&QDoubleSpinBox::valueChanged), this, [this](const double value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationRefreshRate = value;
                this->update_current_preset_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_DOUBLE

        #define OVERLOAD_INT static_cast<void (QSpinBox::*)(int)>

        connect(ui->spinBox_resolutionX, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this](const int value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationResolution.w = std::min(MAX_CAPTURE_WIDTH, unsigned(value));
                this->update_current_preset_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        connect(ui->spinBox_resolutionY, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this](const int value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationResolution.h = std::min(MAX_CAPTURE_HEIGHT, unsigned(value));
                this->update_current_preset_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_INT

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
            if (this->currentPreset)
            {
                this->currentPreset->name = text.toStdString();
                this->update_current_preset_list_text();
            }
        });

        connect(ui->checkBox_activatorRefreshRate, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->doubleSpinBox_refreshRateValue->setEnabled(checked);
            ui->comboBox_refreshRateComparison->setEnabled(checked);
            ui->pushButton_refreshRateSeparator->setEnabled(checked);

            if (this->currentPreset)
            {
                this->currentPreset->activatesWithRefreshRate = checked;
                this->update_current_preset_list_text();
            }

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->checkBox_activatorResolution, &QCheckBox::toggled, this, [this](const bool checked)
        {
            ui->spinBox_resolutionX->setEnabled(checked);
            ui->spinBox_resolutionY->setEnabled(checked);
            ui->pushButton_resolutionSeparator->setEnabled(checked);

            if (this->currentPreset)
            {
                this->currentPreset->activatesWithResolution = checked;
                this->update_current_preset_list_text();
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
                this->update_current_preset_list_text();
            }
        });

        connect(ui->pushButton_resolutionSeparator, &QPushButton::clicked, this, [this](void)
        {
            const auto currentResolution = kc_capture_api().get_resolution();

            ui->spinBox_resolutionX->setValue(int(currentResolution.w));
            ui->spinBox_resolutionY->setValue(int(currentResolution.h));
        });

        connect(ui->pushButton_refreshRateSeparator, &QPushButton::clicked, this, [this](void)
        {
            ui->doubleSpinBox_refreshRateValue->setValue(kc_capture_api().get_refresh_rate().value<double>());
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

                this->update_current_preset_list_text();
            }

            this->update_preset_controls_with_current_preset_data();
        });

        connect(this, &VideoParameterDialog::preset_change_requested, this, [this](const unsigned presetId)
        {
            this->currentPreset = kvideopreset_get_preset_ptr(presetId);
            this->update_preset_controls_with_current_preset_data();
            this->update_current_preset_list_text();

            kvideopreset_apply_current_active_preset();
        });

        connect(ui->comboBox_presetList, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this]()
        {
            if (ui->comboBox_presetList->count() <= 0)
            {
                return;
            }

            emit this->preset_change_requested(ui->comboBox_presetList->currentData().toUInt());
        });

        connect(ui->comboBox_refreshRateComparison, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this](const QString &text)
        {
            if (text == "Exact")
            {
                ui->doubleSpinBox_refreshRateValue->setDecimals(refresh_rate_s::numDecimalsPrecision);
            }
            else
            {
                ui->doubleSpinBox_refreshRateValue->setDecimals(0);
            }

            if (this->currentPreset)
            {
                if (text == "Exact")   this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::equals;
                if (text == "Rounded") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::rounded;
                if (text == "Floored") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::floored;
                if (text == "Ceiled")  this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::ceiled;

                this->update_current_preset_list_text();
                kvideopreset_apply_current_active_preset();
            }
        });

        #undef OVERLOAD_QSTRING
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

    // Subscribe to app events.
    {
        ke_events().capture.newVideoMode.subscribe([this]
        {
            if (kc_capture_api().has_signal())
            {
                this->update_preset_control_ranges();
            }
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
    }

    return true;
}

void VideoParameterDialog::assign_presets(const std::vector<video_preset_s*> &presets)
{
    this->remove_all_video_presets_from_list();

    for (const auto *p: presets)
    {
        this->add_video_preset_to_list(p);
    }

    // Sort the preset list alphabetically.
    /// TODO: It would be better to sort the items by (numeric) resolution.
    ui->comboBox_presetList->model()->sort(0);
    ui->comboBox_presetList->setCurrentIndex(0);

    return;
}

void VideoParameterDialog::update_current_preset_list_text(void)
{
    const QString prevText = ui->comboBox_presetList->itemText(ui->comboBox_presetList->currentIndex());
    const QString newText = this->make_preset_list_text(this->currentPreset);

    if (prevText != newText)
    {
        emit this->data_changed();
        ui->comboBox_presetList->setItemText(ui->comboBox_presetList->currentIndex(), this->make_preset_list_text(this->currentPreset));
    }

    return;
}

void VideoParameterDialog::broadcast_current_preset_parameters(void)
{
    if (!this->currentPreset ||
        !CONTROLS_LIVE_UPDATE)
    {
        return;
    }

    this->currentPreset->videoParameters.overallBrightness  = ui->parameterGrid_videoParams->value("Brightness");
    this->currentPreset->videoParameters.overallContrast    = ui->parameterGrid_videoParams->value("Contrast");
    this->currentPreset->videoParameters.redBrightness      = ui->parameterGrid_videoParams->value("Red br.");
    this->currentPreset->videoParameters.redContrast        = ui->parameterGrid_videoParams->value("Red ct.");
    this->currentPreset->videoParameters.greenBrightness    = ui->parameterGrid_videoParams->value("Green br.");
    this->currentPreset->videoParameters.greenContrast      = ui->parameterGrid_videoParams->value("Green ct.");
    this->currentPreset->videoParameters.blueBrightness     = ui->parameterGrid_videoParams->value("Blue br.");
    this->currentPreset->videoParameters.blueContrast       = ui->parameterGrid_videoParams->value("Blue ct.");
    this->currentPreset->videoParameters.blackLevel         = ui->parameterGrid_videoParams->value("Black level");
    this->currentPreset->videoParameters.horizontalPosition = ui->parameterGrid_videoParams->value("Hor. position");
    this->currentPreset->videoParameters.horizontalScale    = ui->parameterGrid_videoParams->value("Hor. size");
    this->currentPreset->videoParameters.phase              = ui->parameterGrid_videoParams->value("Phase");
    this->currentPreset->videoParameters.verticalPosition   = ui->parameterGrid_videoParams->value("Ver. position");

    kvideoparam_preset_video_params_changed(this->currentPreset->id);

    return;
}

QString VideoParameterDialog::make_preset_list_text(const video_preset_s *const preset)
{
    QStringList text;

    if (preset->activatesWithShortcut)
    {
        text << QString("(Ctrl+F%1)").arg(QString::number(ui->comboBox_shortcutSecondKey->currentIndex() + 1));
    }

    if (preset->activatesWithResolution)
    {
        text << QString("%1 \u00d7 %2").arg(QString::number(preset->activationResolution.w))
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
        text << QString("\"%1\"").arg(QString::fromStdString(preset->name));
    }

    QString combinedText = text.join(" - ");

    if (combinedText.isEmpty())
    {
        combinedText = "(Empty preset)";
    }

    return combinedText;
}

void VideoParameterDialog::remove_all_video_presets_from_list(void)
{
    if (ui->comboBox_presetList->count() <= 0)
    {
        return;
    }

    this->currentPreset = nullptr;
    ui->comboBox_presetList->clear();
    emit this->preset_list_became_empty();

    return;
}

unsigned VideoParameterDialog::find_preset_idx_in_list(const unsigned presetId)
{
    int presetListIdx = -1;

    for (int i = 0; i < ui->comboBox_presetList->count(); i++)
    {
        const int itemPresetId = ui->comboBox_presetList->itemData(i).toUInt();

        if (kvideopreset_get_preset_ptr(itemPresetId)->id == presetId)
        {
            presetListIdx = i;
            break;
        }
    }

    k_assert((presetListIdx >= 0), "Invalid list index.");

    return unsigned(presetListIdx);
}

void VideoParameterDialog::remove_video_preset_from_list(const video_preset_s *const preset)
{
    if (ui->comboBox_presetList->count() <= 0)
    {
        return;
    }

    ui->comboBox_presetList->removeItem(this->find_preset_idx_in_list(preset->id));

    emit this->data_changed();

    if (!ui->comboBox_presetList->count())
    {
        this->currentPreset = nullptr;
        emit this->preset_list_became_empty();
    }

    return;
}

void VideoParameterDialog::add_video_preset_to_list(const video_preset_s *const preset)
{
    if (ui->comboBox_presetList->count() == 0)
    {
        emit this->preset_list_no_longer_empty();
    }

    // Make sure there would be no duplicate preset IDs in the preset list.
    for (int i = 0; i < ui->comboBox_presetList->count(); i++)
    {
        const unsigned itemPresetId = ui->comboBox_presetList->itemData(i).toUInt();
        k_assert((itemPresetId != preset->id), "Detected a duplicate preset id.");
    }

    const QString presetText = this->make_preset_list_text(preset);

    ui->comboBox_presetList->addItem(presetText, preset->id);

    // Select the item we added (we assume it was added to the bottom of the list).
    {
        const unsigned newIdx = (ui->comboBox_presetList->count() - 1);

        // If the list isn't already at the index, we'll make it so, which in turn
        // is expected to automatically emit a preset_change_requested() signal.
        if (ui->comboBox_presetList->currentIndex() != newIdx)
        {
            ui->comboBox_presetList->setCurrentIndex(newIdx);
        }
        else
        {
            emit this->preset_change_requested(preset->id);
        }
    }

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
        const video_signal_parameters_s def = kc_capture_api().get_default_video_signal_parameters();

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
            const auto &currentParams = this->currentPreset->videoParameters;

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
