/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS display wrapper
 *
 * Implements VCS's display interface using Qt, i.e. wraps the interface's
 * functions around the Qt-based GUI.
 *
 */

#include <QApplication>
#include <QMessageBox>
#include <cassert>
#include <thread>
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "capture/alias.h"
#include "common/log/log.h"

// We'll want to avoid accessing the GUI via non-GUI threads, so let's assume
// the thread that creates this unit is the GUI thread. We'll later compare
// against this id to detect out-of-GUI-thread access.
static const std::thread::id NATIVE_THREAD_ID = std::this_thread::get_id();

// Qt wants a QApplication object around for the GUI to function.
namespace app_n
{
    static int ARGC = 1;
    static char NAME[] = "VCS";
    static char *ARGV = NAME;
    static QApplication *const APP = new QApplication(ARGC, &ARGV);
}

// The window we'll display the program in. Also owns the various sub-dialogs, etc.
static MainWindow *WINDOW = nullptr;

void kd_clear_filter_graph(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->clear_filter_graph();
    }

    return;
}

void kd_recalculate_filter_graph_chains(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->recalculate_filter_graph_chains();
    }

    return;
}

void kd_disable_output_size_controls(const bool areDisabled)
{
    if (WINDOW != nullptr)
    {
        WINDOW->disable_output_size_controls(areDisabled);
    }

    return;
}

FilterGraphNode* kd_add_filter_graph_node(const filter_type_enum_e &filterType,
                                          const u8 *const initialParameterValues)
{
    if (WINDOW != nullptr)
    {
        return WINDOW->add_filter_graph_node(filterType, initialParameterValues);
    }

    return nullptr;
}

void kd_acquire_output_window(void)
{
    INFO(("Acquiring the display."));

    WINDOW = new MainWindow;
    WINDOW->show();

    return;
}

void kd_clear_aliases(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->clear_known_aliases();
    }

    return;
}

void kd_add_alias(const mode_alias_s a)
{
    if (WINDOW != nullptr)
    {
        WINDOW->signal_new_known_alias(a);
    }

    return;
}

void kd_set_video_presets_filename(const std::string &filename)
{
    if (WINDOW != nullptr)
    {
        //WINDOW->signal_new_video_presets_source_file(filename);
    }

    return;
}

void kd_update_video_mode_params(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_video_mode_params();
    }

    return;
}

void kd_set_filter_graph_source_filename(const std::string &sourceFilename)
{
    if (WINDOW != nullptr)
    {
        WINDOW->set_filter_graph_source_filename(sourceFilename);
    }

    return;
}

void kd_set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions)
{
    if (WINDOW != nullptr)
    {
        //WINDOW->set_filter_graph_options(graphOptions);
    }

    return;
}

void kd_update_capture_signal_info(void)
{
    if (WINDOW != nullptr)
    {
        //WINDOW->update_capture_signal_info();
    }

    return;
}

void kd_set_capture_signal_reception_status(const bool receivingASignal)
{
    if (WINDOW != nullptr)
    {
        if (receivingASignal)
        {
            WINDOW->set_capture_info_as_receiving_signal();
        }
        else WINDOW->set_capture_info_as_no_signal();
    }

    return;
}

bool kd_add_log_entry(const log_entry_s e)
{
    if (WINDOW != nullptr)
    {
        WINDOW->add_gui_log_entry(e);
        return true;
    }

    return false;
}

void kd_release_output_window(void)
{
    INFO(("Releasing the display."));

    if (WINDOW == nullptr)
    {
        DEBUG(("Expected the display to have been acquired before releasing it. "
               "Ignoring this call."));
    }
    else
    {
        delete WINDOW; WINDOW = nullptr;
        delete app_n::APP;
    }

    return;
}

void kd_spin_event_loop(void)
{
    k_assert(WINDOW != nullptr,
             "Expected the display to have been acquired before accessing it for events processing. ");

    WINDOW->update_gui_state();

    return;
}

void kd_set_video_recording_is_active(const bool isActive)
{
    if (WINDOW != nullptr)
    {
        //WINDOW->set_recording_is_active(isActive);
    }

    return;
}

void kd_update_output_window_title(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_window_title();
    }

    return;
}

void kd_update_output_window_size(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_window_size();
    }

    return;
}

void kd_refresh_filter_chains(void)
{
    k_assert(0, "An unimplemented function was called.");

    /// TODO.

    return;
}

void kd_load_video_presets(const std::string &filename)
{
    k_assert(WINDOW, "Tried to query the display before it had been initialized.");

    WINDOW->video_presets_dialog()->load_presets_from_file(filename);
}

void kd_load_filter_graph(const std::string &filename)
{
    k_assert(WINDOW, "Tried to query the display before it had been initialized.");

    WINDOW->filter_graph_dialog()->load_graph_from_file(QString::fromStdString(filename));
}

void kd_load_aliases(const std::string &filename)
{
    k_assert(WINDOW, "Tried to query the display before it had been initialized.");

    WINDOW->alias_resolutions_dialog()->load_aliases_from_file(QString::fromStdString(filename));
}

bool kd_is_fullscreen(void)
{
    k_assert(WINDOW != nullptr, "Tried to query the display before it had been initialized.");

    return WINDOW->isFullScreen();
}

void kd_redraw_output_window(void)
{
    if (WINDOW == nullptr)
    {
        DEBUG(("Expected the display to have been acquired before accessing it for redraw. "
               "Ignoring this call."));
    }
    else
    {
        WINDOW->redraw();
    }

    return;
}

void kd_show_headless_info_message(const char *const title,
                                   const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Information);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    INFO(("%s", msg));

    return;
}

// Displays the given error in a message box that isn't tied to a particular window
// of the program. Useful for giving out e.g. startup error messages for things that
// occur before the GUI has been initialized.
//
void kd_show_headless_error_message(const char *const title,
                                    const char *const msg)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
        mb.setText(msg);
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    NBENE(("%s", msg));

    return;
}
void kd_show_headless_assert_error_message(const char *const msg,
                                           const char *const filename,
                                           const uint lineNum)
{
    if (std::this_thread::get_id() == NATIVE_THREAD_ID)
    {
        QMessageBox mb;
        mb.setWindowTitle("VCS Assertion Error");
        mb.setText("<html>"
                "VCS has come across an unexpected condition in its code. As a precaution, "
                "the program will shut itself down now.<br><br>"
                "<b>Error:</b> " + QString(msg) + "<br>"
                "<b>In file:</b> " + QString(filename) + "<br>"
                "<b>On line:</b> " + QString::number(lineNum) +
                "</html>");
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setIcon(QMessageBox::Critical);
        mb.setDefaultButton(QMessageBox::Ok);

        mb.exec();
    }

    NBENE(("%s %s %d", msg, filename, lineNum));

    return;
}
