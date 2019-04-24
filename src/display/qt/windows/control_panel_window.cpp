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
#include "display/qt/widgets/control_panel_output_widget.h"
#include "display/qt/widgets/control_panel_input_widget.h"
#include "display/qt/widgets/control_panel_about_widget.h"
#include "display/qt/dialogs/filter_sets_list_dialog.h"
#include "display/qt/dialogs/video_and_color_dialog.h"
#include "display/qt/windows/control_panel_window.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
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

ControlPanel::ControlPanel(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ControlPanel)
{
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
            emit new_programwide_style_file(filename);
            this->update_tab_widths();
        });
    }

    // Set up the contents of the 'Input' tab.
    {
        inputWidget = new ControlPanelInputWidget;
        ui->tab_input->layout()->addWidget(inputWidget);

        connect(inputWidget, &ControlPanelInputWidget::open_video_adjust_dialog,
                       this, [this]
        {
            this->open_video_adjust_dialog();
        });

        connect(inputWidget, &ControlPanelInputWidget::open_alias_dialog,
                       this, [this]
        {
            this->open_alias_dialog();
        });
    }

    // Set up the contents of the 'Output' tab.
    {
        outputWidget = new ControlPanelOutputWidget;
        ui->tab_output->layout()->addWidget(outputWidget);

        connect(outputWidget, &ControlPanelOutputWidget::open_antitear_dialog,
                        this, [this]
        {
            this->open_antitear_dialog();
        });

        connect(outputWidget, &ControlPanelOutputWidget::open_filter_sets_dialog,
                        this, [this]
        {
            this->open_filter_sets_dialog();
        });

        connect(outputWidget, &ControlPanelOutputWidget::open_overlay_dialog,
                        this, [this]
        {
            emit open_overlay_dialog();
        });

        connect(outputWidget, &ControlPanelOutputWidget::set_filtering_enabled,
                        this, [this](const bool state)
        {
            kf_set_filtering_enabled(state);

            k_assert(filterSetsDlg != nullptr, "");
            filterSetsDlg->signal_filtering_enabled(state);
        });

        connect(outputWidget, &ControlPanelOutputWidget::set_renderer,
                        this, [this](const QString &rendererName)
        {
            emit set_renderer(rendererName);
        });
    }

    // Set up the contents of the 'Record' tab.
    {
        recordWidget = new ControlPanelRecordWidget;
        ui->tab_record->layout()->addWidget(recordWidget);

        connect(recordWidget, &ControlPanelRecordWidget::set_output_size_controls_enabled,
                        this, [this](const bool state)
        {
            outputWidget->set_output_size_controls_enabled(state);
        });

        connect(recordWidget, &ControlPanelRecordWidget::update_output_window_title,
                        this, [this]
        {
            emit update_output_window_title();
        });

        connect(recordWidget, &ControlPanelRecordWidget::update_output_window_size,
                        this, [this]
        {
            emit update_output_window_size();
        });
    }

    // Set the GUI controls to their proper initial values.
    {
        ui->treeWidget_logList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        // Restore persistent settings.
        {
            // Control panel.
            this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "control_panel", size()).toSize());
            ui->tabWidget->setCurrentIndex(kpers_value_of(INI_GROUP_CONTROL_PANEL, "tab", 0).toUInt());

            // Log tab.
            ui->checkBox_logEnabled->setChecked(kpers_value_of(INI_GROUP_LOG, "enabled", 1).toBool());
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
        kpers_set_value(INI_GROUP_CONTROL_PANEL, "tab", ui->tabWidget->currentIndex());
        kpers_set_value(INI_GROUP_GEOMETRY, "control_panel", size());
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

void ControlPanel::update_output_framerate(const uint fps,
                                           const bool hasMissedFrames)
{
    outputWidget->update_output_framerate(fps, hasMissedFrames);

    return;
}

void ControlPanel::set_capture_info_as_no_signal()
{
    inputWidget->set_capture_info_as_no_signal();

    outputWidget->set_output_info_enabled(false);

    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->set_controls_enabled(false);

    return;
}

void ControlPanel::set_capture_info_as_receiving_signal()
{
    inputWidget->set_capture_info_as_receiving_signal();

    outputWidget->set_output_info_enabled(true);

    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->set_controls_enabled(true);

    return;
}

void ControlPanel::update_output_resolution_info(void)
{
    outputWidget->update_output_resolution_info();

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
        k_assert(videocolorDlg != nullptr, "");
        videocolorDlg->notify_of_new_capture_signal();

        inputWidget->update_capture_signal_info();

        outputWidget->update_capture_signal_info();
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
    inputWidget->activate_capture_res_button(buttonIdx);

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
    return outputWidget->is_overlay_enabled();
}

// Adjusts the output scale value in the GUI by a pre-set step size in the given
// direction. Note that this will automatically trigger a change in the actual
// scaler output size as well.
//
void ControlPanel::adjust_output_scaling(const int dir)
{
    outputWidget->adjust_output_scaling(dir);

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

void ControlPanel::open_video_adjust_dialog(void)
{
    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->show();
    videocolorDlg->activateWindow();
    videocolorDlg->raise();

    return;
}

void ControlPanel::open_alias_dialog(void)
{
    k_assert(aliasDlg != nullptr, "");
    aliasDlg->show();
    aliasDlg->activateWindow();
    aliasDlg->raise();

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
    outputWidget->toggle_overlay();

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
