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

VideoParameterDialog::VideoParameterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoParameterDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(this->dialogBaseTitle);
    this->setWindowFlags(Qt::Window);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Video parameters...
        {
            QMenu *presetMenu = new QMenu("Video presets", this->menubar);

            connect(presetMenu->addAction("Add preset"), &QAction::triggered, this, [=]
            {
                this->add_video_preset_to_list(kvideopreset_create_preset());
            });

            presetMenu->addSeparator();

            connect(presetMenu->addAction("Save presets as..."), &QAction::triggered, this, [=]
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

            connect(presetMenu->addAction("Load presets..."), &QAction::triggered, this, [=]
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

            presetMenu->addSeparator();

            connect(presetMenu->addAction("Delete preset"), &QAction::triggered, this, [=]
            {
                if (this->currentPreset)
                {
                    const auto presetToDelete = this->currentPreset;
                    this->remove_video_preset_from_list(presetToDelete);
                    kvideopreset_remove_preset(presetToDelete->id);
                }
            });

            connect(presetMenu->addAction("Delete all presets"), &QAction::triggered, this, [=]
            {
                this->remove_all_video_presets_from_list();
                kvideopreset_remove_all_presets();
            });

            this->menubar->addMenu(presetMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Set the GUI controls to their proper initial values.
    {
        ui->groupBox_activation->setEnabled(false);
        ui->groupBox_presetList->setEnabled(false);
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
            }
        });

        connect(ui->spinBox_resolutionY, OVERLOAD_INT(&QSpinBox::valueChanged), this, [this](const int value)
        {
            if (this->currentPreset)
            {
                this->currentPreset->activationResolution.h = std::min(MAX_OUTPUT_HEIGHT, unsigned(value));
                this->update_current_present_list_text();
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
        });

        connect(ui->comboBox_refreshRateComparison, OVERLOAD_QSTRING(&QComboBox::currentIndexChanged), this, [this](const QString &text)
        {
            if (text == "Exactly") ui->doubleSpinBox_refreshRateValue->setDecimals(refresh_rate_s::numDecimalsPrecision);
            else ui->doubleSpinBox_refreshRateValue->setDecimals(0);

            if (this->currentPreset)
            {
                if (text == "Exactly") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::equals;
                if (text == "Rounded") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::rounded;
                if (text == "Floored") this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::floored;
                if (text == "Ceiled")  this->currentPreset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::ceiled;

                this->update_current_present_list_text();
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
                                this->update_current_preset_parameters();
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
        this->set_video_parameter_graph_enabled(kpers_value_of(INI_GROUP_OUTPUT, "video_parameters", kf_is_filtering_enabled()).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "video_parameters", this->size()).toSize());
    }

    return;
}

VideoParameterDialog::~VideoParameterDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OUTPUT, "video_parameters", this->isEnabled);
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

void VideoParameterDialog::update_current_preset_parameters(void)
{
    video_signal_parameters_s p;

    p.overallBrightness  = ui->spinBox_colorBright->value();
    p.overallContrast    = ui->spinBox_colorContr->value();
    p.redBrightness      = ui->spinBox_colorBrightRed->value();
    p.redContrast        = ui->spinBox_colorContrRed->value();
    p.greenBrightness    = ui->spinBox_colorBrightGreen->value();
    p.greenContrast      = ui->spinBox_colorContrGreen->value();
    p.blueBrightness     = ui->spinBox_colorBrightBlue->value();
    p.blueContrast       = ui->spinBox_colorContrBlue->value();
    p.blackLevel         = ui->spinBox_videoBlackLevel->value();
    p.horizontalPosition = ui->spinBox_videoHorPos->value();
    p.horizontalScale    = ui->spinBox_videoHorSize->value();
    p.phase              = ui->spinBox_videoPhase->value();
    p.verticalPosition   = ui->spinBox_videoVerPos->value();

    kvideopreset_update_preset_parameters(this->currentPreset->id, p);

    return;
}

QString VideoParameterDialog::make_preset_list_text(const video_preset_s *const preset)
{
    QStringList text;

    if (preset->activatesWithShortcut)
    {
        text << QString("F%1").arg(QString::number(ui->comboBox_shortcutSecondKey->currentIndex() + 1));
    }

    if (preset->activatesWithResolution)
    {
        text << QString("%1 x %2").arg(QString::number(preset->activationResolution.w))
                                  .arg(QString::number(preset->activationResolution.h));
    }

    if (preset->activatesWithRefreshRate)
    {
        if (this->currentPreset->refreshRateComparator == video_preset_s::refresh_rate_comparison_e::equals)
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
    ui->groupBox_videoParameters->setEnabled(false);
    ui->lineEdit_presetName->clear();

    this->currentPreset = nullptr;

    ui->comboBox_presetList->clear();

    return;
}

void VideoParameterDialog::remove_video_preset_from_list(const video_preset_s *const preset)
{
    if (ui->comboBox_presetList->count() <= 0)
    {
        return;
    }

    // If this is the last preset to be removed, disabled the dialog's preset
    // controls.
    if ((ui->comboBox_presetList->count() == 1))
    {
        ui->groupBox_activation->setEnabled(false);
        ui->groupBox_presetList->setEnabled(false);
        ui->groupBox_videoParameters->setEnabled(false);
        ui->lineEdit_presetName->clear();

        this->currentPreset = nullptr;
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
        ui->groupBox_videoParameters->setEnabled(true);
    }

    const QString presetText = this->make_preset_list_text(preset);

    ui->comboBox_presetList->addItem(presetText, preset->id);

    // Sort the list alphabetically.
    ui->comboBox_presetList->model()->sort(0);

    ui->comboBox_presetList->setCurrentText(presetText);

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

    return;
}

void VideoParameterDialog::set_video_parameter_graph_enabled(const bool enabled)
{
    this->isEnabled = enabled;

    if (!this->isEnabled)
    {
        emit this->video_parameter_graph_disabled();
    }
    else
    {
        emit this->video_parameter_graph_enabled();
    }

    kf_set_filtering_enabled(this->isEnabled);

    kd_update_output_window_title();

    return;
}

bool VideoParameterDialog::is_video_parameter_graph_enabled(void)
{
    return this->isEnabled;
}
