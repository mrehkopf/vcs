/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS display wrapper
 *
 * A convenience wrapper for GUI functionality.
 *
 */

#include <QApplication>
#include <QMessageBox>
#include <assert.h>
#include "../../capture/capture.h"
#include "../../common/globals.h"
#include "../../filter/filter.h"
#include "../../common/log.h"
#include "d_window.h"

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

extern int UPDATE_LATENCY_PEAK;
extern int UPDATE_LATENCY_AVG;

void kd_acquire_display(void)
{
    INFO(("Acquiring the display."));

    WINDOW = new MainWindow;
    WINDOW->show();

    return;
}

void kd_clear_known_aliases(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->clear_known_aliases();
    }

    return;
}

void kd_clear_known_modes(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->clear_known_modes();
    }

    return;
}

void kd_signal_new_known_alias(const mode_alias_s a)
{
    if (WINDOW != nullptr)
    {
        WINDOW->signal_new_known_alias(a);
    }

    return;
}

void kd_signal_new_known_mode(const resolution_s r)
{
    if (WINDOW != nullptr)
    {
        WINDOW->signal_new_known_mode(r);
    }

    return;
}

void kd_signal_new_mode_settings_source_file(const QString &filename)
{
    if (WINDOW != nullptr)
    {
        WINDOW->signal_new_mode_settings_source_file(filename);
    }

    return;
}

void kd_update_gui_filter_set_idx(const int idx)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_current_filter_set_idx(idx);
    }

    return;
}

void kd_update_gui_video_params(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_video_params();
    }

    return;
}

void kd_update_gui_input_signal_info(const input_signal_s &s)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_input_signal_info(s);
    }

    return;
}

void kd_mark_gui_input_no_signal(const bool state)
{
    if (WINDOW != nullptr)
    {
        if (state == true)
        {
            WINDOW->set_input_info_as_no_signal();
        }
        else
        {
            WINDOW->set_input_info_as_receiving_signal();
        }
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

void kd_release_display(void)
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

void kd_process_ui_events(void)
{
    k_assert(WINDOW != nullptr,
             "Expected the display to have been acquired before accessing it for events processing. ");
    WINDOW->update_gui_state();

    return;
}

void kd_update_gui_output_framerate_info(const u32 fps,
                                         const bool missedFrames)
{
    k_assert(WINDOW != nullptr,
             "Expected the display to have been acquired before accessing it for events processing. ");
    WINDOW->update_output_framerate(fps, missedFrames);

    return;
}

void kd_update_gui_recording_metainfo(void)
{
    k_assert(WINDOW != nullptr,
             "Expected the display to have been acquired before accessing it for events processing. ");
    WINDOW->update_recording_metainfo();

    return;
}

void kd_update_display_size(void)
{
    if (WINDOW != nullptr)
    {
        WINDOW->update_window_size();
    }

    return;
}

int kd_peak_system_latency(void)
{
    return UPDATE_LATENCY_PEAK;
}

int kd_average_system_latency(void)
{
    return UPDATE_LATENCY_AVG;
}

bool kd_is_fullscreen(void)
{
    k_assert(WINDOW != nullptr, "Tried to query the display before it had been initialized.");

    return WINDOW->isFullScreen();
}

void kd_update_display(void)
{
    if (WINDOW == nullptr)
    {
        NBENE(("Expected the display to have been acquired before accessing it for redraw. "
               "Ignoring this call."));
    }
    else
    {
        WINDOW->refresh();

        WINDOW->measure_framerate();
    }

    return;
}

void kd_show_headless_info_message(const char *const title,
                                   const char *const msg)
{
    QMessageBox mb;
    mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
    mb.setText(msg);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setIcon(QMessageBox::Information);
    mb.setDefaultButton(QMessageBox::Ok);

    mb.exec();

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
    QMessageBox mb;
    mb.setWindowTitle(strlen(title) == 0? "VCS has this to say" : title);
    mb.setText(msg);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setIcon(QMessageBox::Critical);
    mb.setDefaultButton(QMessageBox::Ok);

    mb.exec();

    NBENE(("%s", msg));

    return;
}
void kd_show_headless_assert_error_message(const char *const msg)
{
    QMessageBox mb;
    mb.setWindowTitle("VCS Assertion Error");
    mb.setText("VCS has come across an unexpected condition in its code that it has "
               "not yet been programmed to deal with. As a precaution, VCS will shut itself "
               "down now.\n\nThe following additional information was attached:\n\""
               + QString(msg) + "\"\n\nIf you ran VCS from a console window, further "
               "diagnostics should appear there once VCS has terminated.");
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setIcon(QMessageBox::Critical);
    mb.setDefaultButton(QMessageBox::Ok);

    mb.exec();

    NBENE(("%s", msg));

    return;
}
