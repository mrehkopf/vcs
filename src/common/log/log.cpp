/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS log
 *
 * Logs given entires into the console or the GUI.
 *
 */

#include <thread>
#include <deque>
#include <stdarg.h>
#include "capture/capture.h"
#include "capture/capture_api.h"
#include "common/propagate/app_events.h"
#include "display/display.h"
#include "common/globals.h"
#include "common/log/log.h"

// Set to false to ignore any log events submitted.
static bool LOGGING_ENABLED = true;

// We'll only accept logging from the logger's own thread.
static const std::thread::id NATIVE_LOG_THREAD = std::this_thread::get_id();

// Log entries submitted for logging but which can't immediately be logged will
// be placed here to wait.
static std::deque<log_entry_s> LOG_CACHE;

// How many entries we've logged.
static uint TOTAL_NUM_LOG_ENTRIES = 0;

void klog_set_logging_enabled(const bool state)
{
    if (state)
    {
        LOGGING_ENABLED = true;
        INFO(("Logging has been enabled."));
    }
    else
    {
        INFO(("[Info] Logging has been disabled by request. Re-enable it to see "
               "further messages."));
        LOGGING_ENABLED = false;
    }

    return;
}

void klog_initialize(void)
{
    ke_events().capture.newVideoMode.subscribe([]
    {
        const auto resolution = kc_capture_api().get_resolution();

        INFO(("New video mode: %u x %u @ %.3f Hz.", resolution.w, resolution.h, kc_capture_api().get_refresh_rate().value<double>()));
    });

    return;
}

void log(const char *const type, const char *const msg, va_list args)
{
    // Only allow logging from the main thread.
    if (std::this_thread::get_id() != NATIVE_LOG_THREAD)
    {
        return;
    }

    // If the user has turned logging off, don't output anything. Except if
    // the program is exiting - then output the last messages of the exit.
    if (!LOGGING_ENABLED &&
        !PROGRAM_EXIT_REQUESTED)
    {
        return;
    }

    char buf[1024];
    vsnprintf(buf, NUM_ELEMENTS(buf), msg, args);

    log_entry_s entry;
    entry.id = TOTAL_NUM_LOG_ENTRIES;
    entry.type = type;
    entry.message = buf;

    LOG_CACHE.push_back(entry);
    TOTAL_NUM_LOG_ENTRIES++;

    // Output the entry into the console.
    printf("[%-5s] %s\n", type, buf);

    // Try and dump the entry/entries into the GUI. If we can't, just
    // give up for now and we'll try again next time.
    while (!LOG_CACHE.empty())
    {
        if (!PROGRAM_EXIT_REQUESTED &&
            kd_add_log_entry(LOG_CACHE.front()))
        {
            LOG_CACHE.pop_front();
        }
        else
        {
            return;
        }
    }

    return;
}

void klog_log_error(const char *const msg, ...)
{
    va_list args;
    va_start(args, msg);
        log("Error", msg, args);
    va_end(args);

    return;
}

void klog_log_info(const char *const msg, ...)
{
    va_list args;
    va_start(args, msg);
        log("Info", msg, args);
    va_end(args);

    return;
}

void klog_log_debug(const char *const msg, ...)
{
    va_list args;
    va_start(args, msg);
        log("Debug", msg, args);
    va_end(args);

    return;
}
