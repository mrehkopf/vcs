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
#include "record/record.h"
#include "scaler/scaler.h"
#include "common/timer/timer.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/core/core.hpp>
#endif

// The arguments taken by scaling functions.
#define SCALER_FUNC_PARAMS u8 *const pixelData, const resolution_s &srcResolution, const resolution_s &dstResolution

vcs_event_c<const resolution_s&> ks_evNewOutputResolution;

// The most recent captured frame has now been processed and is ready for display.
vcs_event_c<const captured_frame_s&> ks_evNewScaledImage;

// The number of frames processed (see newFrame) in the last second.
vcs_event_c<unsigned> ks_evFramesPerSecond;

struct image_scaler_s
{
    // The public name of the scaler. Shown in the GUI etc.
    std::string name;

    // The function that executes the scaler with the given pixels.
    void (*scale)(SCALER_FUNC_PARAMS);
};

// For keeping track of the number of frames scaled per second.
static unsigned NUM_FRAMES_SCALED_PER_SECOND = 0;

// The available scaling filters.
void s_scaler_nearest(SCALER_FUNC_PARAMS);
void s_scaler_linear(SCALER_FUNC_PARAMS);
void s_scaler_area(SCALER_FUNC_PARAMS);
void s_scaler_cubic(SCALER_FUNC_PARAMS);
void s_scaler_lanczos(SCALER_FUNC_PARAMS);
static const std::vector<image_scaler_s> KNOWN_SCALERS =
#ifdef USE_OPENCV
    {{"Nearest", &s_scaler_nearest},
     {"Linear",  &s_scaler_linear},
     {"Area",    &s_scaler_area},
     {"Cubic",   &s_scaler_cubic},
     {"Lanczos", &s_scaler_lanczos}};
#else
    {{"Nearest", &s_scaler_nearest}};
#endif

static const image_scaler_s *CUR_UPSCALER = nullptr;
static const image_scaler_s *CUR_DOWNSCALER = nullptr;

// The frame buffer where scaled frames are to be placed.
static captured_frame_s FRAME_BUFFER;

// Scratch buffers.
static heap_mem<u8> COLORCONV_BUFFER;
static heap_mem<u8> TMP_BUFFER;

 // The frame buffer's target bit depth.
static const u32 OUTPUT_BIT_DEPTH = 32;

// By default, the base resolution for scaling is the resolution of the input
// frame. But the user can also provide an override resolution that takes the
// place of the base resolution.
static resolution_s RESOLUTION_OVERRIDE = {640, 480};
static bool IS_RESOLUTION_OVERRIDE_ENABLED = false;

// To which aspect ratio we should force the base resolution.
static scaler_aspect_ratio_e ASPECT_RATIO = scaler_aspect_ratio_e::native;
static bool IS_ASPECT_RATIO_ENABLED = true;

// An additional multiplier to be applied to the base resolution.
static double SCALING_MULTIPLIER = 1;
static bool IS_SCALING_MULTIPLIER_ENABLED = false;

// Returns the aspect ratio (e.g. 4:3) of the given resolution.
static std::pair<unsigned, unsigned> resolution_to_aspect(const resolution_s &r)
{
    const int gcd = std::__gcd(r.w, r.h);

    return {(r.w / gcd),
            (r.h / gcd)};
}

void ks_set_aspect_ratio(const scaler_aspect_ratio_e ratio)
{
    ASPECT_RATIO = ratio;

    return;
}

scaler_aspect_ratio_e ks_aspect_ratio(void)
{
    return ASPECT_RATIO;
}

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

bool ks_is_aspect_ratio_enabled(void)
{
    return IS_ASPECT_RATIO_ENABLED;
}

#if USE_OPENCV
// Returns a resolution corresponding to srcResolution scaled up to dstResolution but
// maintaining srcResolution's aspect ratio according to the scaler's current aspect
// mode.
//
static resolution_s padded_resolution(const resolution_s &srcResolution, const resolution_s &dstResolution)
{
    const auto aspect = [srcResolution]()->std::pair<int, int>
    {
        switch (ASPECT_RATIO)
        {
            case scaler_aspect_ratio_e::native: return resolution_to_aspect(srcResolution);
            case scaler_aspect_ratio_e::all_4_3: return {4, 3};
            case scaler_aspect_ratio_e::traditional_4_3:
            {
                if ((srcResolution.w == 720 && srcResolution.h == 400) ||
                    (srcResolution.w == 640 && srcResolution.h == 400) ||
                    (srcResolution.w == 320 && srcResolution.h == 200))
                {
                    return {4, 3};
                }
                else
                {
                    return resolution_to_aspect(srcResolution);
                }
            }
            default: k_assert(0, "Unknown aspect mode."); return resolution_to_aspect(srcResolution);
        }
    }();
    const double aspectRatio = (aspect.first / (double)aspect.second);
    uint w = std::round(dstResolution.h * aspectRatio);
    uint h = dstResolution.h;
    if (w > dstResolution.w)
    {
        const double aspectRatio = (aspect.first / (double)aspect.second);
        w = dstResolution.w;
        h = std::round(dstResolution.w * aspectRatio);
    }

    return {w, h, OUTPUT_BIT_DEPTH};
}

// Returns border padding sizes for cv::copyMakeBorder()
//
static cv::Vec4i border_padding(const resolution_s &paddedRes, const resolution_s &dstResolution)
{
    cv::Vec4i p;

    p[0] = ((dstResolution.h - paddedRes.h) / 2);     // Top.
    p[1] = ((dstResolution.h - paddedRes.h + 1) / 2); // Bottom.
    p[2] = ((dstResolution.w - paddedRes.w) / 2);     // Left.
    p[3] = ((dstResolution.w - paddedRes.w + 1) / 2); // Right.

    return p;
}

// Copies src into dsts and adds a border of the given size.
//
void copy_with_border(const cv::Mat &src, cv::Mat &dst, const cv::Vec4i &borderSides)
{
    cv::copyMakeBorder(src, dst, borderSides[0], borderSides[1], borderSides[2], borderSides[3], cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    return;
}

// Scales the given pixel data using OpenCV.
//
void opencv_scale(u8 *const pixelData,
                  u8 *const outputBuffer,
                  const resolution_s &srcResolution,
                  const resolution_s &dstResolution,
                  const cv::InterpolationFlags interpolator)
{
    cv::Mat scratch = cv::Mat(srcResolution.h, srcResolution.w, CV_8UC4, pixelData);
    cv::Mat output = cv::Mat(dstResolution.h, dstResolution.w, CV_8UC4, outputBuffer);

    if (ks_is_aspect_ratio_enabled())
    {
        const resolution_s paddedRes = padded_resolution(srcResolution, dstResolution);
        cv::Mat tmp = cv::Mat(paddedRes.h, paddedRes.w, CV_8UC4, TMP_BUFFER.data());

        if ((paddedRes.h == dstResolution.h) &&
            (paddedRes.w == dstResolution.w))
        {
            // No padding is needed, so we can resize directly into the output buffer.
            cv::resize(scratch, output, output.size(), 0, 0, interpolator);
        }
        else
        {
            cv::resize(scratch, tmp, tmp.size(), 0, 0, interpolator);
            copy_with_border(tmp, output, border_padding(paddedRes, dstResolution));
        }
    }
    else
    {
        cv::resize(scratch, output, output.size(), 0, 0, interpolator);
    }

    return;
}

#endif

void s_scaler_nearest(SCALER_FUNC_PARAMS)
{
    k_assert((srcResolution.bpp == 32) && (dstResolution.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        opencv_scale(pixelData, FRAME_BUFFER.pixels.data(), srcResolution, dstResolution, cv::INTER_NEAREST);
    #else
        double deltaW = (srcResolution.w / double(dstResolution.w));
        double deltaH = (srcResolution.h / double(dstResolution.h));
        u8 *const dst = FRAME_BUFFER.pixels.ptr();

        for (uint y = 0; y < dstResolution.h; y++)
        {
            for (uint x = 0; x < dstResolution.w; x++)
            {
                const uint dstIdx = ((x + y * dstResolution.w) * 4);
                const uint srcIdx = ((uint(x * deltaW) + uint(y * deltaH) * srcResolution.w) * 4);

                memcpy(&dst[dstIdx], &pixelData[srcIdx], 4);
            }
        }
    #endif

    return;
}

void s_scaler_linear(SCALER_FUNC_PARAMS)
{
    k_assert((srcResolution.bpp == 32) && (dstResolution.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        opencv_scale(pixelData, FRAME_BUFFER.pixels.data(), srcResolution, dstResolution, cv::INTER_LINEAR);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void s_scaler_area(SCALER_FUNC_PARAMS)
{
    k_assert((srcResolution.bpp == 32) && (dstResolution.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        opencv_scale(pixelData, FRAME_BUFFER.pixels.data(), srcResolution, dstResolution, cv::INTER_AREA);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void s_scaler_cubic(SCALER_FUNC_PARAMS)
{
    k_assert((srcResolution.bpp == 32) && (dstResolution.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        opencv_scale(pixelData, FRAME_BUFFER.pixels.data(), srcResolution, dstResolution, cv::INTER_CUBIC);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void s_scaler_lanczos(SCALER_FUNC_PARAMS)
{
    k_assert((srcResolution.bpp == 32) && (dstResolution.bpp == 32),
             "This filter requires 32-bit source and target color.")

    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        opencv_scale(pixelData, FRAME_BUFFER.pixels.data(), srcResolution, dstResolution, cv::INTER_LANCZOS4);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

// Replaces OpenCV's default error handler.
//
int cv_error_handler(int status, const char* func_name,
                     const char* err_msg, const char* file_name, int line, void* userdata)
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

void ks_initialize_scaler(void)
{
    INFO(("Initializing the scaler subsystem."));

    #if USE_OPENCV
        cv::redirectError(cv_error_handler);
    #endif

    COLORCONV_BUFFER.allocate(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Scaler color conversion buffer");
    TMP_BUFFER.allocate(MAX_NUM_BYTES_IN_OUTPUT_FRAME, "Scaler scratch buffer");

    FRAME_BUFFER.pixels.allocate(MAX_NUM_BYTES_IN_OUTPUT_FRAME, "Scaler output buffer");
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.r = {0, 0, 0};

    ks_set_upscaling_filter(KNOWN_SCALERS.at(0).name);
    ks_set_downscaling_filter(KNOWN_SCALERS.at(0).name);

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
    TMP_BUFFER.release();

    return;
}

// Converts the given non-BGRA frame into the BGRA format.
void s_convert_frame_to_bgra(const captured_frame_s &frame)
{
    // RGB888 frames are already stored in BGRA format.
    if (frame.pixelFormat == capture_pixel_format_e::rgb_888)
    {
        return;
    }

    #ifdef USE_OPENCV
        u32 conversionType = 0;
        const u32 numColorChan = (frame.r.bpp / 8);

        cv::Mat input = cv::Mat(frame.r.h, frame.r.w, CV_MAKETYPE(CV_8U,numColorChan), frame.pixels.data());
        cv::Mat colorConv = cv::Mat(frame.r.h, frame.r.w, CV_8UC4, COLORCONV_BUFFER.data());

        k_assert(!COLORCONV_BUFFER.is_null(),
                 "Was asked to convert a frame's color depth, but the color conversion buffer "
                 "was null.");

        if (frame.pixelFormat == capture_pixel_format_e::rgb_565)
        {
            conversionType = CV_BGR5652BGRA;
        }
        else if (frame.pixelFormat == capture_pixel_format_e::rgb_555)
        {
            conversionType = CV_BGR5552BGRA;
        }
        else // Unknown type, try to guesstimate it.
        {
            NBENE(("Detected an unknown output pixel format (depth: %u) while converting a frame to BGRA. Attempting to guess its type...",
                   frame.r.bpp));

            if (frame.r.bpp == 32)
            {
                conversionType = CV_RGBA2BGRA;
            }
            if (frame.r.bpp == 24)
            {
                conversionType = CV_BGR2BGRA;
            }
            else
            {
                conversionType = CV_BGR5652BGRA;
            }
        }

        cv::cvtColor(input, colorConv, conversionType);
    #else
        (void)frame;
        k_assert(0, "Was asked to convert the frame to BGRA, but OpenCV had been disabled in the build. Can't do it.");
    #endif

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
            goto done;
        }
        else if (outputRes.w > MAX_OUTPUT_WIDTH ||
                 outputRes.h > MAX_OUTPUT_HEIGHT)
        {
            NBENE(("Was asked to scale a frame with an output size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                    outputRes.w, outputRes.h, MAX_OUTPUT_WIDTH, MAX_OUTPUT_HEIGHT));
            goto done;
        }
        else if (pixelData == nullptr)
        {
            NBENE(("Was asked to scale a null frame. Ignoring it."));
            goto done;
        }
        else if (frame.pixelFormat != kc_get_capture_pixel_format())
        {
            NBENE(("Was asked to scale a frame whose pixel format differed from the expected. Ignoring it."));
            goto done;
        }
        else if (frame.r.bpp > MAX_OUTPUT_BPP)
        {
            NBENE(("Was asked to scale a frame with a color depth (%u bits) higher than that allowed (%u bits). Ignoring it.",
                   frame.r.bpp, MAX_OUTPUT_BPP));
            goto done;
        }
        else if (frame.r.w < minres.w ||
                 frame.r.h < minres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) smaller than the minimum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, minres.w, minres.h));
            goto done;
        }
        else if (frame.r.w > maxres.w ||
                 frame.r.h > maxres.h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, maxres.w, maxres.h));
            goto done;
        }
        else if (FRAME_BUFFER.pixels.is_null())
        {
            goto done;
        }
    }

    // If needed, convert the color data to BGRA, which is what the scaling filters
    // expect to receive. Note that this will only happen if the frame's bit depth
    // doesn't match with the expected value - a frame with the same bit depth but
    // different arrangement of the color channels would not get converted to the
    // proper order.
    if (frame.r.bpp != OUTPUT_BIT_DEPTH)
    {
        s_convert_frame_to_bgra(frame);
        frameRes.bpp = 32;

        pixelData = COLORCONV_BUFFER.data();
    }

    pixelData = kat_anti_tear(pixelData, frameRes);

    /// TODO: If anti-tearing has visualization options turned on, we'd ideally
    /// draw them AFTER applying filtering.
    kf_apply_filter_chain(pixelData, frameRes);

    // Scale the frame to the desired output size.
    {
        // If no need to scale, just copy the data over.
        if ((!IS_ASPECT_RATIO_ENABLED || ASPECT_RATIO == scaler_aspect_ratio_e::native) &&
            frameRes.w == outputRes.w &&
            frameRes.h == outputRes.h)
        {
            memcpy(FRAME_BUFFER.pixels.data(), pixelData, FRAME_BUFFER.pixels.size_check(frameRes.w * frameRes.h * (frameRes.bpp / 8)));
        }
        else
        {
            const image_scaler_s *scaler;

            if ((frameRes.w < outputRes.w) ||
                (frameRes.h < outputRes.h))
            {
                scaler = CUR_UPSCALER;
            }
            else
            {
                scaler = CUR_DOWNSCALER;
            }

            if (!scaler)
            {
                NBENE(("Upscale or downscale filter is null. Refusing to scale."));

                outputRes = frameRes;
                memcpy(FRAME_BUFFER.pixels.data(), pixelData, FRAME_BUFFER.pixels.size_check(frameRes.w * frameRes.h * (frameRes.bpp / 8)));
            }
            else
            {
                scaler->scale(pixelData, frameRes, outputRes);
            }
        }

        if ((FRAME_BUFFER.r.w != outputRes.w) ||
            (FRAME_BUFFER.r.h != outputRes.h))
        {
            ks_evNewOutputResolution.fire(outputRes);
            FRAME_BUFFER.r = outputRes;
        }

        ks_evNewScaledImage.fire(ks_frame_buffer());
    }

    done:
    return;
}

void ks_set_base_resolution_enabled(const bool enabled)
{
    IS_RESOLUTION_OVERRIDE_ENABLED = enabled;
    kd_update_output_window_size();

    return;
}

void ks_set_aspect_ratio_enabled(const bool state)
{
    IS_ASPECT_RATIO_ENABLED = state;
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

const captured_frame_s& ks_frame_buffer(void)
{
    return FRAME_BUFFER;
}

// Returns a list of GUI-displayable names of the scaling filters that're
// available.
//
std::vector<std::string> ks_scaling_filter_names(void)
{
    std::vector<std::string> names;

    for (uint i = 0; i < KNOWN_SCALERS.size(); i++)
    {
        names.push_back(KNOWN_SCALERS[i].name);
    }

    return names;
}

// Returns a scaling filter matching the given name.
//
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

const std::string& ks_upscaling_filter_name(void)
{
    k_assert(CUR_UPSCALER != nullptr,
             "Tried to get the name of a null upscale filter.");

    return CUR_UPSCALER->name;
}

const std::string& ks_downscaling_filter_name(void)
{
    k_assert(CUR_UPSCALER != nullptr,
             "Tried to get the name of a null downscale filter.")

    return CUR_DOWNSCALER->name;
}

void ks_set_upscaling_filter(const std::string &name)
{
    const auto newScaler = scaler_for_name_string(name);

    if (CUR_UPSCALER != newScaler)
    {
        CUR_UPSCALER = newScaler;
    }

    return;
}

void ks_set_downscaling_filter(const std::string &name)
{
    const auto newScaler = scaler_for_name_string(name);

    if (CUR_DOWNSCALER != newScaler)
    {
        CUR_DOWNSCALER = newScaler;
    }

    return;
}
