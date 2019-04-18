/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef LOG_H_
#define LOG_H_

#include <string>
#include "../common/types.h"

struct log_entry_s
{
    // The index of this entry in the master list of log entries.
    uint id;

    // Whether this is a debug, error, or info entry.
    std::string type;

    std::string message;
};

void klog_log_error(const char *const msg, ...);

void klog_log_debug(const char *const msg, ...);

void klog_log_info(const char *const msg, ...);

void klog_set_logging_enabled(const bool state);

#endif
