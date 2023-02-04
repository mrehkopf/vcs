/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * Logs given entires into the VCS console.
 *
 */

#include <thread>
#include <deque>
#include <stdarg.h>
#include "capture/capture.h"
#include "common/propagate/vcs_event.h"
#include "display/display.h"
#include "common/globals.h"
#include "common/log/log.h"

// We'll only accept logging from the logger's own thread.
static const std::thread::id NATIVE_LOG_THREAD = std::this_thread::get_id();

static void log(const std::string &type, const char *const msg, va_list args)
{
    if (std::this_thread::get_id() != NATIVE_LOG_THREAD)
    {
        return;
    }

    const std::string terminalColorResetCode = "\x1B[0m";
    const std::string terminalColorCode = ([&type]()->std::string
    {
        if (type == "Error")
        {
            return "\x1B[31m";
        }
        else if (type == "Debug")
        {
            return "\x1B[36m";
        }
        else if (type == "Assert")
        {
            return "\x1B[33m";
        }
        else
        {
            return "\x1B[0m";
        }
    }());

    static char buf[2048];
    vsnprintf(buf, NUM_ELEMENTS(buf), msg, args);

    printf("%s[%-6s] %s%s\n",
        terminalColorCode.c_str(),
        type.c_str(),
        buf,
        terminalColorResetCode.c_str()
    );

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
        log("Status", msg, args);
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

void klog_log_assert(const char *const msg, ...)
{
    va_list args;

    va_start(args, msg);
        log("Assert", msg, args);
    va_end(args);

    return;
}
