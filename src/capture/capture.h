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

enum class capture_pixel_format_e
{
    RGB_555,
    RGB_565,
    RGB_888,
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
    bool wokeUp;
};

struct capture_video_settings_s
{
    unsigned long horizontalScale;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;
};

struct capture_color_settings_s
{
    long overallBrightness;
    long overallContrast;

    long redBrightness;
    long redContrast;

    long greenBrightness;
    long greenContrast;

    long blueBrightness;
    long blueContrast;
};

// For each input mode (i.e. resolution), the set of parameters that the user
// has defined.
struct video_mode_params_s
{
    resolution_s r;
    capture_color_settings_s color;
    capture_video_settings_s video;
};

#endif
