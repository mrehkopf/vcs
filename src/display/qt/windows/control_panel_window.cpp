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
#include "display/qt/dialogs/video_and_color_dialog.h"
#include "display/qt/windows/control_panel_window.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
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
    filterGraphDlg = new FilterGraphDialog;

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

        connect(outputWidget, &ControlPanelOutputWidget::open_filter_graph_dialog,
                        this, [this]
        {
            this->open_filter_graph_dialog();
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

        connect(recordWidget, &ControlPanelRecordWidget::recording_started,
                        this, [this]
        {
            // Disable any GUI functionality that would let the user change the current
            // output size, since we want to keep the output resolution constant while
            // recording.
            outputWidget->set_output_size_controls_enabled(false);

            emit update_output_window_title();
        }, Qt::DirectConnection);

        connect(recordWidget, &ControlPanelRecordWidget::recording_stopped,
                        this, [this]
        {
            outputWidget->set_output_size_controls_enabled(true);

            emit update_output_window_size();
            emit update_output_window_title();
        }, Qt::DirectConnection);
    }

    return;
}

ControlPanel::~ControlPanel()
{
    // Save persistent settings.
    {
       // kpers_set_value(INI_GROUP_CONTROL_PANEL, "tab", ui->tabWidget->currentIndex());
       // kpers_set_value(INI_GROUP_GEOMETRY, "control_panel", size());
    }

    delete ui;
    ui = nullptr;

    delete aliasDlg;
    aliasDlg = nullptr;

    delete videocolorDlg;
    videocolorDlg = nullptr;

    delete antitearDlg;
    antitearDlg = nullptr;

    delete filterGraphDlg;
    filterGraphDlg = nullptr;

    return;
}

void ControlPanel::restore_persistent_settings(void)
{
    // Restore the control panel's persistent settings.
    this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "control_panel", size()).toSize());
    ui->tabWidget->setCurrentIndex(kpers_value_of(INI_GROUP_CONTROL_PANEL, "tab", 0).toUInt());

    // Restore the related widgets' persistent settings.
    aboutWidget->restore_persistent_settings();
    inputWidget->restore_persistent_settings();
    outputWidget->restore_persistent_settings();
    recordWidget->restore_persistent_settings();

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

        k_assert(filterGraphDlg != nullptr, "");
        filterGraphDlg->close();
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
    return aboutWidget->is_custom_program_styling_enabled();
}

void ControlPanel::notify_of_new_alias(const mode_alias_s a)
{
    k_assert(aliasDlg != nullptr, "");
    aliasDlg->receive_new_alias(a);

    return;
}

void ControlPanel::clear_filter_graph(void)
{
    k_assert(filterGraphDlg != nullptr, "");
    filterGraphDlg->clear_filter_graph();

    return;
}

FilterGraphNode* ControlPanel::add_filter_graph_node(const filter_type_enum_e &filterType,
                                                     const u8 *const initialParameterValues)
{
    k_assert(filterGraphDlg != nullptr, "");
    return filterGraphDlg->add_filter_graph_node(filterType, initialParameterValues);
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

void ControlPanel::update_video_mode_params(void)
{
    k_assert(videocolorDlg != nullptr, "");
    videocolorDlg->update_controls();

    return;
}

void ControlPanel::set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions)
{
    k_assert(filterGraphDlg != nullptr, "");
    filterGraphDlg->set_filter_graph_options(graphOptions);

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

void ControlPanel::notify_of_new_program_version(void)
{
    aboutWidget->notify_of_new_program_version();

    return;
}

void ControlPanel::clear_known_aliases(void)
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
    /// Logging functionality in the GUI is currently fully disabled, so we'll
    /// just ignore this call.

    (void)e;

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

void ControlPanel::set_filter_graph_source_filename(const std::string &sourceFilename)
{
    k_assert(filterGraphDlg != nullptr, "");
    filterGraphDlg->set_filter_graph_source_filename(sourceFilename);

    return;
}

void ControlPanel::open_filter_graph_dialog(void)
{
    k_assert(filterGraphDlg != nullptr, "");
    filterGraphDlg->show();
    filterGraphDlg->activateWindow();
    filterGraphDlg->raise();

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
