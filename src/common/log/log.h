/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_COMMON_LOG_LOG_H
#define VCS_COMMON_LOG_LOG_H

#include <string>
#include "common/types.h"

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
void klog_log_assert(const char *const msg, ...);
void klog_log_info(const char *const msg, ...);

#endif
