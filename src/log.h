/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef LOG_H_
#define LOG_H_

#include <QString>
#include "types.h"

struct log_entry_s
{
    u32 id;             // The index of this entry in the master list of log entries.
    QString date;       // The date (and time) on which this entry was originally made.
    QString type;       // Whether this is a debug, error, or info entry.
    QString message;
};

void klog_log_error(const char *const msg, ...);

void klog_log_debug(const char *const msg, ...);

void klog_log_info(const char *const msg, ...);

void klog_set_logging_enabled(const bool state);

#endif
