/*
 * 2018, 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_ASSERT_H
#define VCS_COMMON_ASSERT_H

#include "display/display.h"
#include "common/log/log.h"

#ifdef NDEBUG
    #error "NDEBUG disables assertions. Assertions are required by design."
#endif
#include <stdexcept>
#include <cassert>

extern bool PROGRAM_EXIT_REQUESTED;

#define k_assert(condition, error_string) \
    if (!(condition))\
    {\
        klog_log_assert("%s:%d \"%s\"", __FILE__, __LINE__, error_string);\
        if (!PROGRAM_EXIT_REQUESTED)\
        {\
            kd_show_headless_assert_error_message(error_string, __FILE__, __LINE__);\
            throw std::runtime_error(error_string);\
        }\
        else\
        {\
            fprintf(stderr, "VCS has crashed while attempting to exit.");\
            std::abort();\
        }\
    }

// Assertions in e.g. performance-critical code.
#ifdef VCS_DEBUG_BUILD
    #define k_assert_optional k_assert
#else
    #define k_assert_optional(...)
#endif

#endif
