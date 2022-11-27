/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS scaler
 *
 * Scales captured frames to match a desired output resolution (for e.g. displaying on screen).
 *
 */

#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath>
#include "anti_tear/anti_tear.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "common/memory/memory.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "common/timer/timer.h"
#include <opencv2/imgproc/imgproc.hpp>

vcs_event_c<const resolution_s&> ks_evNewOutputResolution;
vcs_event_c<const captured_frame_s&> ks_evNewScaledImage;
vcs_event_c<unsigned> ks_evFramesPerSecond;
vcs_event_c<void> ks_evCustomScalerEnabled;
vcs_event_c<void> ks_evCustomScalerDisabled;

// For keeping track of the number of frames scaled per second.
static unsigned NUM_FRAMES_SCALED_PER_SECOND = 0;

// The image scalers available to VCS.
static const std::vector<image_scaler_s> KNOWN_SCALERS =
    {{"Nearest", filter_output_scaler_c::nearest},
     {"Linear",  filter_output_scaler_c::linear},
     {"Area",    filter_output_scaler_c::area},
     {"Cubic",   filter_output_scaler_c::cubic},
     {"Lanczos", filter_output_scaler_c::lanczos}};

static const image_scaler_s *DEFAULT_SCALER = nullptr;

// The frame buffer where scaled frames are to be placed.
static captured_frame_s FRAME_BUFFER;

// Scratch image buffers.
static heap_mem<u8> COLORCONV_BUFFER;

 // The frame buffer's target bit depth.
static const u32 OUTPUT_BIT_DEPTH = 32;

// Whether the current output scaling is done via an output scaling filter that
// the user has specified in a filter chain.
static bool IS_CUSTOM_SCALER_USED = false;
static resolution_s CUSTOM_SCALER_FILTER_RESOLUTION = {0, 0};

// By default, the base resolution for scaling is the resolution of the input
// frame. But the user can also provide an override resolution that takes the
// place of the base resolution.
static resolution_s RESOLUTION_OVERRIDE = {640, 480};
static bool IS_RESOLUTION_OVERRIDE_ENABLED = false;

// An additional multiplier to be applied to the base resolution.
static double SCALING_MULTIPLIER = 1;
static bool IS_SCALING_MULTIPLIER_ENABLED = false;

resolution_s ks_base_resolution(void)
{
    return RESOLUTION_OVERRIDE;
}

// Returns the resolution at which the scaler will output after performing all the actions
// (e.g. relative scaling or aspect ratio correction) that it has been asked to.
//
resolution_s ks_output_resolution(void)
{
    // While recording video, the output resolution is required to stay locked
    // to the video resolution.
    if (krecord_is_recording())
    {
        const auto r = krecord_video_resolution();
        return {r.w, r.h, OUTPUT_BIT_DEPTH};
    }

    if (IS_CUSTOM_SCALER_USED)
    {
        return CUSTOM_SCALER_FILTER_RESOLUTION;
    }

    resolution_s inRes = kc_get_capture_resolution();
    resolution_s outRes = inRes;

    // Base resolution.
    if (IS_RESOLUTION_OVERRIDE_ENABLED)
    {
        outRes = RESOLUTION_OVERRIDE;
    }

    // Scaling.
    if (IS_SCALING_MULTIPLIER_ENABLED)
    {
        outRes.w = round(outRes.w * SCALING_MULTIPLIER);
        outRes.h = round(outRes.h * SCALING_MULTIPLIER);
    }

    // Bounds-check.
    {
        if (outRes.w > MAX_OUTPUT_WIDTH)
        {
            outRes.w = MAX_OUTPUT_HEIGHT;
        }
        else if (outRes.w < MIN_OUTPUT_WIDTH)
        {
            outRes.w = MIN_OUTPUT_WIDTH;
        }

        if (outRes.h > MAX_OUTPUT_HEIGHT)
        {
            outRes.h = MAX_OUTPUT_HEIGHT;
        }
        else if (outRes.h < MIN_OUTPUT_HEIGHT)
        {
            outRes.h = MIN_OUTPUT_HEIGHT;
        }
    }

    outRes.bpp = OUTPUT_BIT_DEPTH;

    return outRes;
}

// Replaces OpenCV's default error handler.
//
static int cv_error_handler(int status, const char* func_name, const char* err_msg, const char* file_name, int line, void* userdata)
{
    NBENE(("OpenCV reports an error: '%s'.", err_msg));
    k_assert(0, "OpenCV reported an error.");

    (void)func_name;
    (void)file_name;
    (void)userdata;
    (void)err_msg;
    (void)status;
    (void)line;

    return 1;
}

bool ks_is_custom_scaler_active(void)
{
    return IS_CUSTOM_SCALER_USED;
}

void ks_initialize_scaler(void)
{
    INFO(("Initializing the scaler subsystem."));

    cv::redirectError(cv_error_handler);

    COLORCONV_BUFFER.allocate(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Scaler color conversion buffer");

    FRAME_BUFFER.pixels.allocate(MAX_NUM_BYTES_IN_OUTPUT_FRAME, "Scaler output buffer");
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.r = {0, 0, 0};

    ks_set_default_scaler(KNOWN_SCALERS.at(0).name);

    kc_evNewCapturedFrame.listen([](const captured_frame_s &frame)
    {
        ks_scale_frame(frame);
    });

    kc_evInvalidSignal.listen([]
    {
        ks_indicate_invalid_signal();
        kd_evDirty.fire();
    });

    kc_evSignalLost.listen([]
    {
        ks_indicate_no_signal();
        kd_evDirty.fire();
    });

    ks_evNewScaledImage.listen([]
    {
        NUM_FRAMES_SCALED_PER_SECOND++;
    });

    kt_timer(1000, [](const unsigned)
    {
        ks_evFramesPerSecond.fire(NUM_FRAMES_SCALED_PER_SECOND);

        NUM_FRAMES_SCALED_PER_SECOND = 0;
    });

    return;
}

void ks_release_scaler(void)
{
    INFO(("Releasing the scaler."));

    COLORCONV_BUFFER.release();
    FRAME_BUFFER.pixels.release();

    return;
}

// Converts the given non-BGRA frame into the BGRA format.
static void convert_frame_to_bgra(const captured_frame_s &frame)
{
    // RGB888 frames are already stored in BGRA format.
    if (frame.pixelFormat == capture_pixel_format_e::rgb_888)
    {
        return;
    }

    u32 conversionType = 0;
    const u32 numColorChan = (frame.r.bpp / 8);

    cv::Mat input = cv::Mat(frame.r.h, frame.r.w, CV_MAKETYPE(CV_8U,numColorChan), frame.pixels.data());
    cv::Mat colorConv = cv::Mat(frame.r.h, frame.r.w, CV_8UC4, COLORCONV_BUFFER.data());

    k_assert(!COLORCONV_BUFFER.is_null(),
             "Was asked to convert a frame's color depth, but the color conversion buffer "
             "was null.");

    if (frame.pixelFormat == capture_pixel_format_e::rgb_565)
    {
        conversionType = cv::COLOR_BGR5652BGRA;
    }
    else if (frame.pixelFormat == capture_pixel_format_e::rgb_555)
    {
        conversionType = cv::COLOR_BGR5552BGRA;
    }
    else // Unknown type, try to guesstimate it.
    {
        NBENE(("Detected an unknown output pixel format (depth: %u) while converting a frame to BGRA. Attempting to guess its type...",
               frame.r.bpp));

        if (frame.r.bpp == 32)
        {
            conversionType = cv::COLOR_RGBA2BGRA;
        }
        if (frame.r.bpp == 24)
        {
            conversionType = cv::COLOR_BGR2BGRA;
        }
        else
        {
            conversionType = cv::COLOR_BGR5652BGRA;
        }
    }

    cv::cvtColor(input, colorConv, conversionType);

    return;
}

// Takes the given image and scales it according to the scaler's current internal
// resolution settings. The scaled image is placed in the scaler's internal buffer,
// not in the source buffer.
//
void ks_scale_frame(const captured_frame_s &frame)
{
    u8 *pixelData = frame.pixels.data();
    resolution_s frameRes = frame.r; /// Temp hack. May want to modify the .bpp value.
    resolution_s outputRes = ks_output_resolution();

    const resolution_s minres = kc_get_device_minimum_resolution();
    const resolution_s maxres = kc_get_device_maximum_resolution();

    // Verify that we have a workable frame.
    {
        if ((frame.r.bpp != 16) &&
            (frame.r.bpp != 24) &&
            (frame.r.bpp != 32))
        {
            NBENE(("Was asked to scale a frame with an incompatible bit depth (%u). Ignoring it.",
                    frame.r.bpp));
            return;
        }
        else if (outputRes.w > MAX_OUTPUT_WIDTH ||
                 outputRes.h > MAX_OUTPUT_HEIGHT)
        {
            NBENE(("Was asked to scale a frame with an output size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                    outputRes.w, outputRes.h, MAX_OUTPUT_WIDTH, MAX_OUTPUT_HEIGHT));
            return;
        }
        else if (pixelData == nullptr)
        {
            NBENE(("Was asked to scale a null frame. Ignoring it."));
            return;
        }
        else if (frame.pixelFormat != kc_get_capture_pixel_format())
        {
            NBENE(("Was asked to scale a frame whose pixel format differed from the expected. Ignoring it."));
            return;
        }
        else if (frame.r.bpp > MAX_OUTPUT_BPP)
        {
            NBENE(("Was asked to scale a frame with a color depth (%u bits) higher than that allowed (%u bits). Ignoring it.",
                   frame.r.bpp, MAX_OUTPUT_BPP));
            return;
        }
        else if (frame.r.w < minres.w ||
                 frame.r.h < minres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) smaller than the minimum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, minres.w, minres.h));
            return;
        }
        else if (frame.r.w > maxres.w ||
                 frame.r.h > maxres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, maxres.w, maxres.h));
            return;
        }
        else if (FRAME_BUFFER.pixels.is_null())
        {
            return;
        }
    }

    // If needed, convert the color data to BGRA, which is what the scaling filters
    // expect to receive. Note that this will only happen if the frame's bit depth
    // doesn't match with the expected value - a frame with the same bit depth but
    // different arrangement of the color channels would not get converted to the
    // proper order.
    if (frame.r.bpp != OUTPUT_BIT_DEPTH)
    {
        convert_frame_to_bgra(frame);
        frameRes.bpp = OUTPUT_BIT_DEPTH;
        pixelData = COLORCONV_BUFFER.data();
    }

    pixelData = kat_anti_tear(pixelData, frameRes);

    abstract_filter_c *customScaler = kf_apply_matching_filter_chain(pixelData, frameRes);

    // If the active filter chain provided a custom output scaler, it'll override our
    // default scaler.
    if (customScaler)
    {
        k_assert(
            (customScaler->category() == filter_category_e::output_scaler),
            "Invalid filter category for custom output scaler."
        );

        if (!IS_CUSTOM_SCALER_USED)
        {
            ks_evCustomScalerEnabled.fire();
        }

        IS_CUSTOM_SCALER_USED = true;

        image_s dstImage = {pixelData, frameRes};
        customScaler->apply(&dstImage);
        CUSTOM_SCALER_FILTER_RESOLUTION = dynamic_cast<filter_output_scaler_c*>(customScaler)->output_resolution();
    }
    else
    {
        if (IS_CUSTOM_SCALER_USED)
        {
            ks_evCustomScalerDisabled.fire();
        }

        IS_CUSTOM_SCALER_USED = false;

        // If no need to scale, just copy the data over.
        if (frameRes == outputRes)
        {
            std::memcpy(FRAME_BUFFER.pixels.data(), pixelData, FRAME_BUFFER.pixels.size_check(frameRes.w * frameRes.h * (frameRes.bpp / 8)));
        }
        else
        {
            k_assert(DEFAULT_SCALER, "A default scaler has not been defined.");

            const image_s srcImage = image_s(pixelData, frameRes);
            image_s dstImage = image_s(FRAME_BUFFER.pixels.data(), outputRes);
            DEFAULT_SCALER->apply(srcImage, &dstImage, {0, 0, 0, 0});
        }
    }

    if ((FRAME_BUFFER.r.w != outputRes.w) ||
        (FRAME_BUFFER.r.h != outputRes.h))
    {
        ks_evNewOutputResolution.fire(outputRes);
        FRAME_BUFFER.r = outputRes;
    }

    ks_evNewScaledImage.fire(ks_frame_buffer());

    return;
}

void ks_set_base_resolution_enabled(const bool enabled)
{
    IS_RESOLUTION_OVERRIDE_ENABLED = enabled;
    kd_update_output_window_size();

    return;
}

void ks_set_base_resolution(const resolution_s &r)
{
    RESOLUTION_OVERRIDE = r;
    kd_update_output_window_size();

    return;
}

void ks_set_scaling_multiplier(const double s)
{
    SCALING_MULTIPLIER = s;
    kd_update_output_window_size();

    return;
}

double ks_scaling_multiplier(void)
{
    return SCALING_MULTIPLIER;
}

void ks_set_scaling_multiplier_enabled(const bool enabled)
{
    IS_SCALING_MULTIPLIER_ENABLED = enabled;

    kd_update_output_window_size();

    return;
}

static void clear_frame_buffer(void)
{
    k_assert(!FRAME_BUFFER.pixels.is_null(),
             "Can't access the output buffer: it was unexpectedly null.");

    memset(FRAME_BUFFER.pixels.data(), 0, FRAME_BUFFER.pixels.size_check(MAX_NUM_BYTES_IN_OUTPUT_FRAME));

    return;
}

void ks_indicate_no_signal(void)
{
    clear_frame_buffer();

    return;
}

void ks_indicate_invalid_signal(void)
{
    clear_frame_buffer();

    return;
}

captured_frame_s& ks_frame_buffer(void)
{
    return FRAME_BUFFER;
}

// Returns a list of GUI-displayable names of the scalers that're available.
//
std::vector<std::string> ks_scaler_names(void)
{
    std::vector<std::string> names;

    for (uint i = 0; i < KNOWN_SCALERS.size(); i++)
    {
        names.push_back(KNOWN_SCALERS[i].name);
    }

    return names;
}

static const image_scaler_s* scaler_for_name_string(const std::string &name)
{
    const image_scaler_s *f = nullptr;

    k_assert(!KNOWN_SCALERS.empty(),
             "Could find no scaling filters to search.");

    for (size_t i = 0; i < KNOWN_SCALERS.size(); i++)
    {
        if (KNOWN_SCALERS[i].name == name)
        {
            f = &KNOWN_SCALERS[i];
            goto done;
        }
    }

    f = &KNOWN_SCALERS.at(0);
    NBENE(("Was unable to find a scaler called '%s'. "
           "Defaulting to the first scaler on the list (%s).",
           name.c_str(), f->name.c_str()));

    done:
    return f;
}

void ks_set_default_scaler(const std::string &name)
{
    DEFAULT_SCALER = scaler_for_name_string(name);

    return;
}

const image_scaler_s* ks_default_scaler(void)
{
    return DEFAULT_SCALER;
}
