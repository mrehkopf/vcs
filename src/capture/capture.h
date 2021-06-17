/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_CAPTURE_CAPTURE_H
#define VCS_CAPTURE_CAPTURE_H

#include <vector>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/memory/memory.h"
#include "common/types.h"

struct capture_device_s;

capture_device_s& kc_capture_device(void);
void kc_initialize_capture(void);
void kc_release_capture(void);
bool kc_force_input_resolution(const resolution_s &r);

enum class capture_deinterlacing_mode_e
{
    weave,
    bob,
    field_0,
    field_1,
};

enum class capture_pixel_format_e
{
    rgb_555,
    rgb_565,
    rgb_888,
};

enum class capture_event_e
{
    none,
    sleep,
    signal_lost,
    new_frame,
    new_video_mode,
    invalid_signal,
    invalid_device,
    unrecoverable_error,

    // Total enumerator count; should remain the last item on the list.
    num_enumerators
};

struct captured_frame_s
{
    resolution_s r;

    capture_pixel_format_e pixelFormat;

    heap_bytes_s<u8> pixels;

    // Will be set to true after the frame's data has been processed for
    // display and is no longer needed.
    bool processed = false;
};

struct signal_info_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
};

// Capture video parameters for a given resolution.
struct video_signal_parameters_s
{
    resolution_s r; // For legacy (VCS <= 1.6.5) support.

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
