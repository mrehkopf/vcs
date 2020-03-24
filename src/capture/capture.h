/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <vector>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/memory.h"
#include "common/types.h"

struct capture_api_s;

capture_api_s& kc_capture_api(void);
void kc_initialize_capture(void);
void kc_release_capture(void);

enum class capturePixelFormat_e
{
    rgb_555,
    rgb_565,
    rgb_888,
};

enum class capture_event_e
{
    none,
    sleep,
    no_signal,
    new_frame,
    new_video_mode,
    invalid_signal,
    unrecoverable_error,

    // Total enumerator count; should remain the last item on the list.
    num_enumerators
};

struct captured_frame_s
{
    resolution_s r;

    heap_bytes_s<u8> pixels;

    // Will be set to true after the frame's data has been processed for
    // display and is no longer needed.
    bool processed = false;
};

struct capture_signal_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
};

// Capture video parameters for a given resolution.
struct video_signal_parameters_s
{
    resolution_s r;

    long overallBrightness;
    long overallContrast;

    long redBrightness;
    long redContrast;

    long greenBrightness;
    long greenContrast;

    long blueBrightness;
    long blueContrast;

    unsigned long horizontalScale;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;
};

#endif
