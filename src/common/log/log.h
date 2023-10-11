/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_COMMON_LOG_LOG_H
#define VCS_COMMON_LOG_LOG_H

#include <string>

#define DEBUG(args) (klog_log_debug args)
#define NBENE(args) (klog_log_error args)
#define INFO(args)  (klog_log_info args)

struct log_entry_s
{
    // The index of this entry in the master list of log entries.
    unsigned id;

    // Whether this is a debug, error, or info entry.
    std::string type;

    std::string message;
};

void klog_log_error(const char *const msg, ...);
void klog_log_debug(const char *const msg, ...);
void klog_log_assert(const char *const msg, ...);
void klog_log_info(const char *const msg, ...);

#endif
