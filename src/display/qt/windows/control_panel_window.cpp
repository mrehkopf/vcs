/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS control panel
 *
 * The main UI dialog for controlling VCS. Orchestrates most other dialogs; sub-
 * ordinate only to the main (capture) window.
 *
 */

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QFile>
#include <assert.h>
#include "display/qt/widgets/control_panel_record_widget.h"
#include "display/qt/widgets/control_panel_about_widget.h"
#include "display/qt/dialogs/filter_sets_list_dialog.h"
#include "display/qt/dialogs/video_and_color_dialog.h"
#include "display/qt/windows/control_panel_window.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "filter/anti_tear.h"
#include "common/propagate.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "capture/alias.h"
#include "record/record.h"
#include "filter/filter.h"
#include "common/log.h"
#include "ui_control_panel_window.h"

#if _WIN32
    #include <windows.h>
#endif

static MainWindow *MAIN_WIN = nullptr;

ControlPanel::ControlPanel(MainWindow *const mainWin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControlPanel)
{
    MAIN_WIN = mainWin;
    k_assert(MAIN_WIN != nullptr,
             "Expected a valid main window pointer in the control panel, but got null.");

    ui->setupUi(this);

    this->setWindowTitle("VCS - Control Panel");
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    aliasDlg = new AliasDialog;
    videocolorDlg = new VideoAndColorDialog;
    antitearDlg = new AntiTearDialog;
    filterSetsDlg = new FilterSetsListDialog;

    // Set up the contents of the 'About' tab.
    {
        aboutWidget = new ControlPanelAboutWidget;
        ui->tab_about->layout()->addWidget(aboutWidget);

        // Emitted when the user assigns a new app-wide stylesheet file via the
        // tab's controls.
        connect(aboutWidget, &ControlPanelAboutWidget::new_programwide_style_file,
                       this, [this](const QString &filename)
        {
            MAIN_WIN->apply_programwide_styling(filename);
            this->update_tab_widths();
        });
    }

    // Set up the contents of the 'Record' tab.
    {
        recordWidget = new ControlPanelRecordWidget;
        ui->tab_record->layout()->addWidget(recordWidget);

        connect(recordWidget, &ControlPanelRecordWidget::set_output_size_controls_enabled,
                       this, [this](const bool state)
        {
            ui->frame_outputOverrides->setEnabled(state);
        });

        connect(recordWidget, &ControlPanelRecordWidget::update_output_window_title,
                       this, [this]
        {
            MAIN_WIN->update_window_title();
        });

        connect(recordWidget, &ControlPanelRecordWidget::update_output_window_size,
                       this, [this]
        {
            MAIN_WIN->update_window_size();
        });
    }

    update_stylesheet(MAIN_WIN->styleSheet());

    connect_capture_resolution_buttons();
    fill_output_scaling_filter_comboboxes();
    fill_capture_channel_combobox();
    reset_capture_bit_depth_combobox();

    // Adjust sundry GUI controls to their proper values.
    {
        ui->spinBox_outputResX->setMinimum(MIN_OUTPUT_WIDTH);
        ui->spinBox_outputResY->setMinimum(MIN_OUTPUT_HEIGHT);
        ui->spinBox_outputResX->setMaximum(MAX_OUTPUT_WIDTH);
        ui->spinBox_outputResY->setMaximum(MAX_OUTPUT_HEIGHT);

        ui->treeWidget_logList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        // Restore persistent settings.
        {
            const auto combobox_idx_of_string = [](const QComboBox *const box, const QString &string)
            {
                int idx = box->findText(string, Qt::MatchExactly);
                if (idx == -1)
                {
                    NBENE(("Unable to find a combo-box item called '%s'. Defaulting to the box's first entry.",
                           string.toStdString().c_str()));

                    idx = 0;
                }

                return idx;
            };

            // Control panel.
            this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "control_panel", size()).toSize());
            ui->tabWidget->setCurrentIndex(kpers_value_of(INI_GROUP_CONTROL_PANEL, "tab", 0).toUInt());

            // Log tab.
            ui->checkBox_logEnabled->setChecked(kpers_value_of(INI_GROUP_LOG, "enabled", 1).toBool());

            // Output tab.
            ui->comboBox_outputUpscaleFilter->setCurrentIndex(
                        combobox_idx_of_string(ui->comboBox_outputUpscaleFilter, kpers_value_of(INI_GROUP_OUTPUT, "upscaler", "Linear").toString()));
            ui->comboBox_outputDownscaleFilter->setCurrentIndex(
                        combobox_idx_of_string(ui->comboBox_outputDownscaleFilter, kpers_value_of(INI_GROUP_OUTPUT, "downscaler", "Linear").toString()));
            ui->checkBox_customFiltering->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "custom_filtering", 0).toBool());
            ui->checkBox_outputKeepAspectRatio->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "keep_aspect", true).toBool());
            ui->comboBox_outputAspectMode->setCurrentIndex(
                        combobox_idx_of_string(ui->comboBox_outputAspectMode, kpers_value_of(INI_GROUP_OUTPUT, "aspect_mode", "Native").toString()));
            ui->checkBox_forceOutputRes->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_output_size", false).toBool());
            ui->spinBox_outputResX->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().width());
            ui->spinBox_outputResY->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().height());
            ui->checkBox_forceOutputScale->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_relative_scale", false).toBool());
            ui->spinBox_outputScale->setValue(kpers_value_of(INI_GROUP_OUTPUT, "relative_scale", 100).toInt());
            ui->comboBox_renderer->setCurrentIndex(
                        combobox_idx_of_string(ui->comboBox_renderer, kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString()));

            // Miscellaneous.
            ui->checkBox_outputAntiTear->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", false).toBool());
        }
    }

    /// For now, don't show the log tab. I might remove it completely, as I'm
    /// not sure how useful it is in the GUI, and not having it makes things a
    /// bit cleaner visually.
    ui->tabWidget->removeTab(3);
    ui->checkBox_logEnabled->setChecked(true); // So logging still goes through to the terminal.

    return;
}

ControlPanel::~ControlPanel()
{
    // Save the current settings.
    {
        // Miscellaneous.
        kpers_set_value(INI_GROUP_LOG, "enabled", ui->checkBox_logEnabled->isChecked());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", ui->checkBox_outputAntiTear->isChecked());
        kpers_set_value(INI_GROUP_CONTROL_PANEL, "tab", ui->tabWidget->currentIndex());
        kpers_set_value(INI_GROUP_GEOMETRY, "control_panel", size());

        // Output tab.
        kpers_set_value(INI_GROUP_OUTPUT, "upscaler", ui->comboBox_outputUpscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "downscaler", ui->comboBox_outputDownscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "custom_filtering", ui->checkBox_customFiltering->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "keep_aspect", ui->checkBox_outputKeepAspectRatio->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "aspect_mode", ui->comboBox_outputAspectMode->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "force_output_size", ui->checkBox_forceOutputRes->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "output_size", QSize(ui->spinBox_outputResX->value(), ui->spinBox_outputResY->value()));
        kpers_set_value(INI_GROUP_OUTPUT, "force_relative_scale", ui->checkBox_forceOutputScale->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "relative_scale", ui->spinBox_outputScale->value());
        kpers_set_value(INI_GROUP_OUTPUT, "renderer", ui->comboBox_renderer->currentText());
    }

    delete ui;
    ui = nullptr;

    delete aliasDlg;
    aliasDlg = nullptr;

    delete videocolorDlg;
    videocolorDlg = nullptr;

    delete antitearDlg;
    antitearDlg = nullptr;

    delete filterSetsDlg;
    filterSetsDlg = nullptr;

    return;
}

void ControlPanel::update_stylesheet(const QString &stylesheet)
{
    k_assert((MAIN_WIN && aliasDlg && videocolorDlg && antitearDlg && filterSetsDlg && aboutWidget),
             "Can't reload the control panel's stylesheet - some of the recipient widgets are null.");

    this->setStyleSheet(stylesheet);
    aliasDlg->setStyleSheet(stylesheet);
    videocolorDlg->setStyleSheet(stylesheet);
    antitearDlg->setStyleSheet(stylesheet);
    filterSetsDlg->setStyleSheet(stylesheet);
    aboutWidget->setStyleSheet(stylesheet);

    return;
}

void ControlPanel::keyPressEvent(QKeyEvent *event)
{
    // Don't allow ESC to close the control panel.
    if (event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        QDialog::keyPressEvent(event);
    }

    return;
}

void ControlPanel::resizeEvent(QResizeEvent *)
{
    update_tab_widths();

    return;
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    // Don't allow the control panel to be closed unless the entire program is
    // closing.
    if (!PROGRAM_EXIT_REQUESTED)
    {
        event->ignore();
    }
    else
    {
        k_assert(videocolorDlg != nullptr, "");
        videocolorDlg->close();

        k_assert(aliasDlg != nullptr, "");
        aliasDlg->close();

        k_assert(antitearDlg != nullptr, "");
        antitearDlg->close();

        k_assert(filterSetsDlg != nullptr, "");
        filterSetsDlg->close();
    }

    return;
}

// Resize the tab widget's tabs so that together they span the tab bar's entire width.
void ControlPanel::update_tab_widths(void)
{
    if (custom_program_styling_enabled())
    {
        const uint tabWidth = (ui->tabWidget->width() / ui->tabWidget->count());
        const uint lastTabWidth = (ui->tabWidget->width() - (tabWidth * (ui->tabWidget->count() - 1)));
        ui->tabWidget->setStyleSheet("QTabBar::tab {width: " + QString::number(tabWidth) + "px;}"
                                     "QTabBar::tab:last {width: " + QString::number(lastTabWidth) + "px;}");
    }
    else
    {
        ui->tabWidget->setStyleSheet("");
    }

    return;
}

bool ControlPanel::custom_program_styling_enabled(void)
{
    k_assert(aboutWidget, "Attempted to access the About tab widget before it had been initialized.");
    return aboutWidget->custom_program_styling_enabled();
}

void ControlPanel::notify_of_new_alias(const mode_alias_s a)
{
    k_assert(aliasDlg != nullptr, "");
    aliasDlg->receive_new_alias(a);

    return;
}

void ControlPanel::notify_of_new_mode_settings_source_file(const QString &filename)
{
    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->receive_new_mode_settings_filename(filename);
}

void ControlPanel::fill_capture_channel_combobox()
{
    block_widget_signals_c b(ui->comboBox_inputChannel);

    ui->comboBox_inputChannel->clear();

    for (int i = 0; i < kc_hardware().meta.num_capture_inputs(); i++)
    {
        ui->comboBox_inputChannel->addItem(QString("Channel #%1").arg((i + 1)));
    }

    ui->comboBox_inputChannel->setCurrentIndex(INPUT_CHANNEL_IDX);

    // Lock the input channel selector if only one channel is available.
    if (ui->comboBox_inputChannel->count() == 1)
    {
        ui->comboBox_inputChannel->setEnabled(false);
    }

    return;
}

// Adds the names of the available scaling filters to the GUI's list.
//
void ControlPanel::fill_output_scaling_filter_comboboxes()
{
    const std::vector<std::string> filterNames = ks_list_of_scaling_filter_names();
    k_assert(!filterNames.empty(), "Expected to receive a list of scaling filters, but got an empty list.");

    block_widget_signals_c b1(ui->comboBox_outputUpscaleFilter);
    block_widget_signals_c b2(ui->comboBox_outputDownscaleFilter);

    ui->comboBox_outputUpscaleFilter->clear();
    ui->comboBox_outputDownscaleFilter->clear();

    for (const auto &name: filterNames)
    {
        ui->comboBox_outputUpscaleFilter->addItem(QString::fromStdString(name));
        ui->comboBox_outputDownscaleFilter->addItem(QString::fromStdString(name));
    }

    return;
}

void ControlPanel::update_output_framerate(const uint fps,
                                           const bool hasMissedFrames)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to update GUI output info while there was no signal."));
    }
    else
    {
        ui->label_outputFPS->setText((fps == 0)? "Unknown" : QString("%1").arg(fps));
        ui->label_outputProcessTime->setText(hasMissedFrames? "Dropping frames" : "No problem");
    }

    return;
}

void ControlPanel::set_capture_info_as_no_signal()
{
    ui->label_captInputResolution->setText("n/a");

    if (kc_is_invalid_signal())
    {
        ui->label_captInputSignal->setText("Invalid signal");
    }
    else
    {
        ui->label_captInputSignal->setText("No signal");
    }

    set_capture_controls_enabled(false);
    set_output_controls_enabled(false);

    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->set_controls_enabled(false);

    return;
}

void ControlPanel::set_capture_info_as_receiving_signal()
{
    set_capture_controls_enabled(true);
    set_output_controls_enabled(true);

    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->set_controls_enabled(true);

    return;
}

void ControlPanel::set_capture_controls_enabled(const bool state)
{
    ui->frame_inputForceButtons->setEnabled(state);
    ui->pushButton_inputAdjustVideoColor->setEnabled(state);
    ui->comboBox_frameSkip->setEnabled(state);
    ui->comboBox_bitDepth->setEnabled(state);
    ui->label_captInputResolution->setEnabled(state);

    return;
}

// Sets the output tab's control buttons etc. as enabled or disabled.
//
void ControlPanel::set_output_controls_enabled(const bool state)
{
    ui->label_outputFPS->setEnabled(state);
    ui->label_outputProcessTime->setEnabled(state);

    ui->label_outputFPS->setText("n/a");
    ui->label_outputProcessTime->setText("n/a");

    return;
}

// Signal to the control panel that it should update itself on the current output
// resolution.
//
void ControlPanel::update_output_resolution_info(void)
{
    const resolution_s r = ks_output_resolution();

    ui->label_outputResolution->setText(QString("%1 x %2").arg(r.w).arg(r.h));

    return;
}

void ControlPanel::update_filter_set_idx(void)
{
    k_assert(filterSetsDlg != nullptr, "");
    filterSetsDlg->update_filter_set_idx();

    return;
}

void ControlPanel::update_filter_sets_list(void)
{
    k_assert(filterSetsDlg != nullptr, "");
    filterSetsDlg->update_filter_sets_list();

    return;
}

void ControlPanel::update_video_params(void)
{
    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->update_controls();

    return;
}

void ControlPanel::update_capture_signal_info(void)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to update GUI input info while there was no signal."));
    }
    else
    {
        const capture_signal_s s = kc_hardware().status.signal();

        k_assert(videocolorDlg != nullptr, "");
        videocolorDlg->notify_of_new_capture_signal();

        // Mark the resolution. If either dimenson is 0, we expect it to be an
        // invalid reading that should be ignored.
        if (s.r.w == 0 || s.r.h == 0)
        {
            ui->label_captInputResolution->setText("n/a");
        }
        else
        {
            QString res = QString("%1 x %2").arg(s.r.w).arg(s.r.h);

            ui->label_captInputResolution->setText(res);
        }

        // Mark the refresh rate. If the value is 0, we expect it to be an invalid
        // reading and that we should ignore it.
        if (s.refreshRate != 0)
        {
            const QString t = ui->label_captInputResolution->text();

            ui->label_captInputResolution->setText(t + QString(", %1 Hz").arg(s.refreshRate));
        }

        ui->label_captInputSignal->setText(s.isDigital? "Digital" : "Analog");

        if (!ui->checkBox_forceOutputRes->isChecked())
        {
            ui->spinBox_outputResX->setValue(s.r.w);
            ui->spinBox_outputResY->setValue(s.r.h);
        }
    }

    return;
}

void ControlPanel::clear_known_aliases()
{
    k_assert(aliasDlg != nullptr, "");
    aliasDlg->clear_known_aliases();

    return;
}

// Simulate the given input button being clicked.
//
void ControlPanel::activate_capture_res_button(const uint buttonIdx)
{
    for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
    {
        QWidget *const w = ui->frame_inputForceButtons->layout()->itemAt(i)->widget();

        /// A bit kludgy, but...
        if (w->objectName().endsWith(QString::number(buttonIdx)))
        {
            parse_capture_resolution_button_press(w);
            return;
        }
    }

    NBENE(("Failed to find input resolution button #%u.", buttonIdx));

    return;
}

// Sets up the buttons for forcing output resolution.
//
void ControlPanel::connect_capture_resolution_buttons()
{
    for (int i = 0; i < ui->frame_inputForceButtons->layout()->count(); i++)
    {
        QWidget *const w = ui->frame_inputForceButtons->layout()->itemAt(i)->widget();

        k_assert(w->objectName().contains("pushButton"), "Expected all widgets in this layout to be pushbuttons.");

        // Store the unique id of this button, so we can later identify it.
        ((QPushButton*)w)->setProperty("butt_id", i);

        // Load in any custom resolutions the user may have set earlier.
        if (kpers_contains(INI_GROUP_INPUT, QString("force_res_%1").arg(i)))
        {
            ((QPushButton*)w)->setText(kpers_value_of(INI_GROUP_INPUT, QString("force_res_%1").arg(i)).toString());
        }

        connect((QPushButton*)w, &QPushButton::clicked,
                           this, [this,w]{this->parse_capture_resolution_button_press(w);});
    }

    return;
}

// Gets called when a button for forcing the input resolution is pressed in the GUI.
// Will then decide which input resolution to force based on which button was pressed.
//
void ControlPanel::parse_capture_resolution_button_press(QWidget *button)
{
    QStringList sl;
    resolution_s res = {0, 0};

    k_assert(button->objectName().contains("pushButton"),
             "Expected a button widget, but received something else.");

    // Query the user for a custom resolution.
    /// TODO. Get a more reliable way.
    if (((QPushButton*)button)->text() == "Other...")
    {
        res.w = 1920;
        res.h = 1080;
        if (ResolutionDialog("Force an input resolution",
                             &res, parentWidget()).exec() == QDialog::Rejected)
        {
            // If the user canceled.
            goto done;
        }

        goto assign_resolution;
    }

    // Extract the resolution from the button name. The name is expected to be
    // of the form e.g. '640 x 480' or '640x480'.
    sl = ((QPushButton*)button)->text().split('x');
    if (sl.size() < 2)
    {
        DEBUG(("Unexpected number of parameters in a button name. Expected at least width and height."));
        goto done;
    }
    else
    {
        res.w = sl.at(0).toUInt();
        res.h = sl.at(1).toUInt();
    }

    // If alt is pressed while clicking the button, let the user specify a new
    // resolution for the button.
    if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
    {
        // Pop up a dialog asking the user for the new resolution.
        if (ResolutionDialog("Assign an input resolution",
                             &res, parentWidget()).exec() != QDialog::Rejected)
        {
            const QString resolutionStr = QString("%1 x %2").arg(res.w).arg(res.h);

            ((QPushButton*)button)->setText(resolutionStr);

            // Save the new resolution into the ini.
            kpers_set_value(INI_GROUP_INPUT,
                            QString("force_res_%1").arg(((QPushButton*)button)->property("butt_id").toUInt()),
                            resolutionStr);

            DEBUG(("Assigned a new resolution (%u x %u) for an input force button.",
                   res.w, res.h));
        }

        goto done;
    }

    assign_resolution:
    DEBUG(("Received a request via the GUI to set the input resolution to %u x %u.", res.w, res.h));
    kpropagate_forced_capture_resolution(res);

    done:
    return;
}

void ControlPanel::add_gui_log_entry(const log_entry_s &e)
{
    // Sanity check, to make sure we've set up the GUI correctly.
    k_assert(ui->treeWidget_logList->columnCount() == 2,
             "Expected the log list to have three columns.");

    QTreeWidgetItem *entry = new QTreeWidgetItem;

    entry->setText(0, QString::fromStdString(e.type));
    entry->setText(1, QString::fromStdString(e.message));

    ui->treeWidget_logList->addTopLevelItem(entry);

    filter_log_entry(entry);

    return;
}

// Initializes the visibility of the given entry based on whether the user has
// selected to show/hide entries of its kind.
//
void ControlPanel::filter_log_entry(QTreeWidgetItem *const entry)
{
    // The column index in the tree that gives the log entry's type.
    const int typeColumn = 0;

    entry->setHidden(true);

    if (ui->checkBox_logInfo->isChecked() &&
        entry->text(typeColumn) == "Info")
    {
        entry->setHidden(false);
    }

    if (ui->checkBox_logDebug->isChecked() &&
        entry->text(typeColumn) == "Debug")
    {
        entry->setHidden(false);
    }

    if (ui->checkBox_logErrors->isChecked() &&
        entry->text(typeColumn) == "N.B.")
    {
        entry->setHidden(false);
    }

    return;
}

void ControlPanel::refresh_log_list_filtering()
{
    const int typeColumn = 1;  // The column index in the tree that gives the log entry's type.
    QList<QTreeWidgetItem*> entries = ui->treeWidget_logList->findItems("*", Qt::MatchWildcard, typeColumn);

    for (auto *entry: entries)
    {
        filter_log_entry(entry);
    }

    return;
}

bool ControlPanel::is_overlay_enabled(void)
{
    return ui->checkBox_outputOverlayEnabled->isChecked();
}

// Adjusts the output scale value in the GUI by a pre-set step size in the given
// direction. Note that this will automatically trigger a change in the actual
// scaler output size as well.
//
void ControlPanel::adjust_output_scaling(const int dir)
{
    const int curValue = ui->spinBox_outputScale->value();
    const int stepSize = ui->spinBox_outputScale->singleStep();

    k_assert((dir == 1 || dir == -1),
             "Expected the parameter for AdjustOutputScaleValue to be either 1 or -1.");

    if (!ui->checkBox_forceOutputScale->isChecked())
    {
        ui->checkBox_forceOutputScale->setChecked(true);
    }

    ui->spinBox_outputScale->setValue(curValue + (stepSize * dir));

    return;
}

void ControlPanel::set_overlay_indicator_checked(const bool checked)
{
    block_widget_signals_c b(ui->checkBox_outputOverlayEnabled);

    ui->checkBox_outputOverlayEnabled->setChecked(checked);

    return;
}

// Queries the current capture input bit depth and sets the GUI combobox selection
// accordingly.
//
void ControlPanel::reset_capture_bit_depth_combobox()
{
    QString depthString = QString("%1-bit").arg(kc_input_color_depth()); // E.g. "24-bit".

    for (int i = 0; i < ui->comboBox_bitDepth->count(); i++)
    {
        if (ui->comboBox_bitDepth->itemText(i).contains(depthString))
        {
            ui->comboBox_bitDepth->setCurrentIndex(i);
            goto done;
        }
    }

    k_assert(0, "Failed to set up the GUI for the current capture bit depth.");

    done:
    return;
}

void ControlPanel::on_checkBox_forceOutputScale_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'forceOutputScale'. It appears to have a third state.");

    ui->spinBox_outputScale->setEnabled(checked);

    if (checked)
    {
        const int s = ui->spinBox_outputScale->value();

        ks_set_output_scaling(s / 100.0);
    }
    else
    {
        ui->spinBox_outputScale->setValue(100);
    }

    ks_set_output_scale_override_enabled(checked);

    return;
}

void ControlPanel::on_checkBox_forceOutputRes_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'outputRes'. It appears to have a third state.");

    ui->spinBox_outputResX->setEnabled(checked);
    ui->spinBox_outputResY->setEnabled(checked);

    ks_set_output_resolution_override_enabled(checked);

    if (!checked)
    {
        resolution_s outr;
        const resolution_s r = kc_hardware().status.capture_resolution();

        ks_set_output_base_resolution(r, true);

        outr = ks_output_base_resolution();
        ui->spinBox_outputResX->setValue(outr.w);
        ui->spinBox_outputResY->setValue(outr.h);
    }

    return;
}

void ControlPanel::on_comboBox_frameSkip_currentIndexChanged(const QString &arg1)
{
    int v = 0;

    if (arg1 == "None")
    {
        v = 0;
    }
    else if (arg1 == "Half")
    {
        v = 1;
    }
    else if (arg1 == "Two thirds")
    {
        v = 2;
    }
    else if (arg1 == "Three quarters")
    {
        v = 3;
    }
    else
    {
        k_assert(0, "Unexpected GUI string for frame-skipping.");
    }

    kc_set_frame_dropping(v);

    return;
}

void ControlPanel::on_comboBox_outputUpscaleFilter_currentIndexChanged(const QString &arg1)
{
    ks_set_upscaling_filter(arg1.toStdString());

    return;
}

void ControlPanel::on_comboBox_outputDownscaleFilter_currentIndexChanged(const QString &arg1)
{
    ks_set_downscaling_filter(arg1.toStdString());

    return;
}

void ControlPanel::on_comboBox_renderer_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "Software")
    {
        INFO(("Renderer: software."));
        MAIN_WIN->set_opengl_enabled(false);
    }
    else if (arg1 == "OpenGL")
    {
        INFO(("Renderer: OpenGL."));
        MAIN_WIN->set_opengl_enabled(true);
    }
    else
    {
        k_assert(0, "Unknown renderer type.");
    }

    return;
}

void ControlPanel::on_checkBox_logEnabled_stateChanged(int arg1)
{
    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'enableLogging'. It appears to have a third state.");

    klog_set_logging_enabled(arg1);

    ui->treeWidget_logList->setEnabled(arg1);

    return;
}

void ControlPanel::on_spinBox_outputScale_valueChanged(int)
{
    real scale = ui->spinBox_outputScale->value() / 100.0;

    ks_set_output_scaling(scale);

    return;
}

void ControlPanel::on_spinBox_outputResX_valueChanged(int)
{
    const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                            (uint)ui->spinBox_outputResY->value(), 0};

    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        return;
    }

    ks_set_output_base_resolution(r, true);

    return;
}

void ControlPanel::on_spinBox_outputResY_valueChanged(int)
{
    const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                            (uint)ui->spinBox_outputResY->value(), 0};

    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        return;
    }

    ks_set_output_base_resolution(r, true);

    return;
}

void ControlPanel::open_video_adjust_dialog(void)
{
    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->show();
    videocolorDlg->activateWindow();
    videocolorDlg->raise();

    return;
}

void ControlPanel::open_antitear_dialog(void)
{
    k_assert(antitearDlg != nullptr, "");
    antitearDlg->show();
    antitearDlg->activateWindow();
    antitearDlg->raise();

    return;
}

void ControlPanel::toggle_overlay(void)
{
    ui->checkBox_outputOverlayEnabled->toggle();

    return;
}

void ControlPanel::open_filter_sets_dialog(void)
{
    k_assert(filterSetsDlg != nullptr, "");
    filterSetsDlg->show();
    filterSetsDlg->activateWindow();
    filterSetsDlg->raise();

    return;
}

void ControlPanel::on_pushButton_inputAdjustVideoColor_clicked()
{
    this->open_video_adjust_dialog();

    return;
}

void ControlPanel::on_comboBox_inputChannel_currentIndexChanged(int index)
{
    // If we fail to set the input channel, revert back to the current one.
    if (!kc_set_input_channel(index))
    {
        block_widget_signals_c b(ui->comboBox_inputChannel);

        NBENE(("Failed to set the input channel to %d. Reverting.", index));
        ui->comboBox_inputChannel->setCurrentIndex(kc_input_channel_idx());
    }

    return;
}

void ControlPanel::on_pushButton_inputAliases_clicked()
{
    k_assert(aliasDlg != nullptr, "");
    aliasDlg->show();
    aliasDlg->activateWindow();
    aliasDlg->raise();

    return;
}

void ControlPanel::on_pushButton_editOverlay_clicked()
{
    k_assert(MAIN_WIN != nullptr, "");
    MAIN_WIN->show_overlay_dialog();

    return;
}

void ControlPanel::on_comboBox_bitDepth_currentIndexChanged(const QString &arg1)
{
    u32 bpp = 0;

    if (arg1.contains("24-bit"))
    {
        bpp = 24;
    }
    else if (arg1.contains("16-bit"))
    {
        bpp = 16;
    }
    else if (arg1.contains("15-bit"))
    {
        bpp = 15;
    }
    else
    {
        k_assert(0, "Unrecognized color depth option in the GUI dropbox.");
    }

    if (!kc_set_input_color_depth(bpp))
    {
        reset_capture_bit_depth_combobox();

        kd_show_headless_error_message("", "Failed to change the capture color depth.\n\n"
                                           "The previous setting has been restored.");
    }

    return;
}

void ControlPanel::on_checkBox_outputAntiTear_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'outputAntiTear'. It appears to have a third state.");

    kat_set_anti_tear_enabled(checked);

    return;
}

void ControlPanel::on_pushButton_antiTearOptions_clicked()
{
    this->open_antitear_dialog();

    return;
}

void ControlPanel::on_pushButton_configureFiltering_clicked()
{
    this->open_filter_sets_dialog();

    return;
}

void ControlPanel::on_checkBox_customFiltering_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'customFiltering'. It appears to have a third state.");

    kf_set_filtering_enabled(checked);

    k_assert(filterSetsDlg != nullptr, "");
    filterSetsDlg->signal_filtering_enabled(checked);

    return;
}

void ControlPanel::on_checkBox_logInfo_toggled(bool)
{
    refresh_log_list_filtering();

    return;
}

void ControlPanel::on_checkBox_logDebug_toggled(bool)
{
    refresh_log_list_filtering();

    return;
}

void ControlPanel::on_checkBox_logErrors_toggled(bool)
{
    refresh_log_list_filtering();

    return;
}

QString ControlPanel::GetString_OutputResolution()
{
    return ui->label_outputResolution->text();
}

QString ControlPanel::GetString_InputResolution()
{
    const resolution_s r = kc_hardware().status.capture_resolution();

    return QString("%1 x %2").arg(r.w).arg(r.h);
}

QString ControlPanel::GetString_InputRefreshRate()
{
    return QString("%1 Hz").arg(kc_hardware().status.signal().refreshRate);
}

QString ControlPanel::GetString_OutputFrameRate()
{
    return ui->label_outputFPS->text();
}

QString ControlPanel::GetString_DroppingFrames()
{
    return ui->label_outputProcessTime->text();
}

QString ControlPanel::GetString_OutputLatency()
{
    return ui->label_outputProcessTime->text();
}

bool ControlPanel::is_mouse_wheel_scaling_allowed()
{
    return (!kd_is_fullscreen() && // In my virtual machine, at least, wheel scaling while in full-screen messes up the full-screen mode.
            !krecord_is_recording());
}

void ControlPanel::update_recording_metainfo(void)
{
    k_assert(recordWidget, "Attempted to access the Record tab widget before it had been initialized.");
    recordWidget->update_recording_metainfo();

    return;
}

void ControlPanel::on_checkBox_outputKeepAspectRatio_stateChanged(int arg1)
{
    ks_set_output_pad_override_enabled((arg1 == Qt::Checked));
    ui->comboBox_outputAspectMode->setEnabled((arg1 == Qt::Checked));

    return;
}

void ControlPanel::on_comboBox_outputAspectMode_currentIndexChanged(const QString &arg1)
{
    INFO(("Setting aspect mode to '%s'.", arg1.toStdString().c_str()));

    if (arg1 == "Native")
    {
        ks_set_aspect_mode(aspect_mode_e::native);
    }
    else if (arg1 == "Traditional 4:3")
    {
        ks_set_aspect_mode(aspect_mode_e::traditional_4_3);
    }
    else if (arg1 == "Always 4:3")
    {
        ks_set_aspect_mode(aspect_mode_e::always_4_3);
    }
    else
    {
        k_assert(0, "Unknown aspect mode.");
    }

    return;
}
