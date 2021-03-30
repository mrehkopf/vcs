/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_COMMON_GLOBALS_H
#define VCS_COMMON_GLOBALS_H

#include "display/display.h"
#include "common/types.h"
#include "common/log/log.h"

#ifdef NDEBUG
    #error "NDEBUG disables assertions. Assertions are required by design."
#endif
#include <stdexcept>
#include <cassert>

const char PROGRAM_NAME[] = "VCS";
const char PROGRAM_VERSION_STRING[] = "2.3.2";

// The minimum and maximum resolution we can output frames in.
const uint MIN_OUTPUT_WIDTH = 160;
const uint MIN_OUTPUT_HEIGHT = 100;
const uint MAX_OUTPUT_WIDTH = 4096;
const uint MAX_OUTPUT_HEIGHT = 2160;
const uint MAX_OUTPUT_BPP = 32;
const u32 MAX_NUM_BYTES_IN_OUTPUT_FRAME = (MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT * u64(MAX_OUTPUT_BPP / 8));

// The minimum and maximum resolution we can accept frames from the capture
// device in.
const uint MIN_CAPTURE_WIDTH = 1;
const uint MIN_CAPTURE_HEIGHT = 1;
const uint MAX_CAPTURE_WIDTH = 1920;
const uint MAX_CAPTURE_HEIGHT = 1080;
const uint MAX_CAPTURE_BPP = 32;
const u32 MAX_NUM_BYTES_IN_CAPTURED_FRAME = (MAX_CAPTURE_WIDTH * MAX_CAPTURE_HEIGHT * u64(MAX_CAPTURE_BPP / 8));

#define NUM_ELEMENTS(array) int((sizeof(array) / sizeof((array)[0])))

#define k_assert(condition, error_string)   if (!(condition))\
                                            {\
                                                DEBUG_(("Assertion failure in %s:%d: \"%s\"", __FILE__, __LINE__, error_string));\
                                                kd_show_headless_assert_error_message(error_string, __FILE__, __LINE__);\
                                                throw std::runtime_error(error_string);\
                                            }

// Assertions in e.g. performance-critical code.
#ifndef RELEASE_BUILD
    #define k_assert_optional k_assert
#else
    #define k_assert_optional(...)
#endif

#define DEBUG_(args)        (printf("[Debug] {%s:%i} ", __FILE__, __LINE__), printf args, printf("\n"), fflush(stdout)) /// Temp hack.
#define DEBUG(args)         (klog_log_debug args)
#define NBENE(args)         (klog_log_error args)
#define INFO(args)          (klog_log_info args)

extern unsigned int FRAME_SKIP;

extern i32 PROGRAM_EXIT_REQUESTED;

extern unsigned int INPUT_CHANNEL_IDX;

#endif
