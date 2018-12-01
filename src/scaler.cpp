/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS scaler
 *
 * Scales captured frames to match a desired output resolution (for e.g. displaying on screen).
 *
 */

#include <QStringList>
#include <QImage>
#include <algorithm>
#include <vector>
#include "anti_tear.h"
#include "capture.h"
#include "display.h"
#include "memory.h"
#include "scaler.h"
#include "common.h"
#include "filter.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/core/core.hpp>
#endif

void s_scaler_nearest(SCALER_FUNC_PARAMS);
void s_scaler_linear(SCALER_FUNC_PARAMS);
void s_scaler_area(SCALER_FUNC_PARAMS);
void s_scaler_cubic(SCALER_FUNC_PARAMS);
void s_scaler_lanczos(SCALER_FUNC_PARAMS);
void s_scaler_gaussian_linear(SCALER_FUNC_PARAMS);
void s_scaler_gaussian_area(SCALER_FUNC_PARAMS);

static const scaling_filter_s *UPSCALE_FILTER = nullptr;
static const scaling_filter_s *DOWNSCALE_FILTER = nullptr;
static const std::vector<scaling_filter_s> SCALING_FILTERS =    // User-facing scaling filters. Note that these names will be shown in the GUI.
#ifdef USE_OPENCV
                {{"Nearest", &s_scaler_nearest},
                 {"Linear",  &s_scaler_linear},
                 {"Area",    &s_scaler_area},
                 {"Cubic",   &s_scaler_cubic},
                 {"Lanczos", &s_scaler_lanczos}};
#else
                {{"Nearest", &s_scaler_nearest}};
#endif

// The pixel buffer where scaled frames are to be placed.
static heap_bytes_s<u8> OUTPUT_BUFFER;

// Scratch buffers.
static heap_bytes_s<u8> COLORCONV_BUFFER;
static heap_bytes_s<u8> TMP_BUFFER;

static resolution_s LATEST_OUTPUT_SIZE = {0};       // The size of the image currently in the scaler's output buffer.

static const u32 OUTPUT_BIT_DEPTH = 32;             // The bit depth we're currently scaling to.

static resolution_s BASE_RESOLUTION = {640, 480, 0};// The size of the capture window, before any other scaling.
static bool FORCE_BASE_RESOLUTION = false;          // If false, the base resolution will track the capture card's output resolution.

static resolution_s PADDED_RESOLUTION = {640, 480, 0};
static bool FORCE_PADDING = false;

static resolution_s ASPECT_RATIO = {1, 1, 0};
static bool FORCE_ASPECT_RATIO = false;

// The multiplier by which to up/downscale the base output resolution.
static real OUTPUT_SCALING = 1;
static bool FORCE_SCALING = false;

resolution_s ks_resolution_to_aspect_ratio(const resolution_s &r)
{
    resolution_s a;
    const int gcd = std::__gcd(r.w, r.h);

    a.w = (r.w / gcd);
    a.h = (r.h / gcd);

    return a;
}

resolution_s ks_output_base_resolution(void)
{
    return BASE_RESOLUTION;
}

resolution_s ks_padded_output_resolution(void)
{
    if (FORCE_PADDING) return PADDED_RESOLUTION;
    else return ks_output_resolution();
}

// Returns the resolution at which the scaler will output after performing all the actions
// (e.g. relative scaling or aspect ratio correction) that it has been asked to.
//
resolution_s ks_output_resolution(void)
{
    real aspect;
    resolution_s inRes = kc_input_signal_info().r;
    resolution_s outRes = inRes;

    // Base resolution.
    if (FORCE_BASE_RESOLUTION)
    {
        outRes = BASE_RESOLUTION;
    }

    // Aspect ratio.
    if (FORCE_ASPECT_RATIO)
    {
        aspect = ((outRes.w / (real)outRes.h) /(ASPECT_RATIO.w /(real)ASPECT_RATIO.h));

        outRes.h = round((real)outRes.h * aspect);
    }

    // Magnification.
    if (FORCE_SCALING)
    {
        outRes.w = round(outRes.w * OUTPUT_SCALING);
        outRes.h = round(outRes.h * OUTPUT_SCALING);
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

bool ks_is_output_padding_enabled(void)
{
    return FORCE_PADDING;
}

void s_scaler_nearest(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::resize(scratch, output, output.size(), 0, 0, cv::INTER_NEAREST);
    #else
        /// TODO. Implement a non-OpenCV nearest scaler so there's a basic fallback.
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_linear(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::resize(scratch, output, output.size(), 0, 0, cv::INTER_LINEAR);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_area(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::resize(scratch, output, output.size(), 0, 0, cv::INTER_AREA);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_cubic(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::resize(scratch, output, output.size(), 0, 0, cv::INTER_CUBIC);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_lanczos(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::resize(scratch, output, output.size(), 0, 0, cv::INTER_LANCZOS4);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_gaussian_linear(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::Mat tmp = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, TMP_BUFFER.ptr());
        cv::resize(scratch, tmp, tmp.size(), 0, 0, cv::INTER_LINEAR);

        // Unsharp mask.
        cv::GaussianBlur(tmp, output, cv::Size(0, 0), 0.52);
        cv::addWeighted(tmp, 1.5, output, -0.5, 0, output);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

void s_scaler_gaussian_area(SCALER_FUNC_PARAMS)
{
    k_assert((sourceRes.bpp == 32) && (targetRes.bpp == 32),
             "This filter requires 32-bit source and target color.")
    if (pixelData == nullptr)
    {
        return;
    }

    #if USE_OPENCV
        cv::Mat scratch = cv::Mat(sourceRes.h, sourceRes.w, CV_8UC4, pixelData);
        cv::Mat output = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, OUTPUT_BUFFER.ptr());
        cv::Mat tmp = cv::Mat(targetRes.h, targetRes.w, CV_8UC4, TMP_BUFFER.ptr());

        cv::resize(scratch, tmp, tmp.size(), 0, 0, cv::INTER_AREA);

        // Unsharp mask.
        cv::GaussianBlur(tmp, output, cv::Size(0, 0), 0.48);
        cv::addWeighted(tmp, 1.5, output, -0.5, 0, output);
    #else
        k_assert(0,
                 "Attempted to use a scaling filter that hasn't been implemented.");
    #endif

    return;
}

uint ks_max_output_bit_depth(void)
{
    return MAX_OUTPUT_BPP;
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
    INFO(("Initializing the scaler."));

    #if USE_OPENCV
        cv::redirectError(cv_error_handler);
    #endif

    OUTPUT_BUFFER.alloc(MAX_FRAME_SIZE, "Scaler output buffer");
    COLORCONV_BUFFER.alloc(MAX_FRAME_SIZE, "Scaler color convertion buffer");
    TMP_BUFFER.alloc(MAX_FRAME_SIZE, "Scaler scratch buffer");

    ks_set_upscaling_filter(SCALING_FILTERS.at(0).name);
    ks_set_downscaling_filter(SCALING_FILTERS.at(0).name);

    return;
}

void ks_release_scaler(void)
{
    INFO(("Releasing the scaler."));

    COLORCONV_BUFFER.release_memory();
    OUTPUT_BUFFER.release_memory();
    TMP_BUFFER.release_memory();

    return;
}

// Converts the given frame to BGRA format.
//
void s_convert_frame_to_bgra(captured_frame_s &frame)
{
    #ifdef USE_OPENCV
        u32 conversionType = 0;
        const u32 numColorChan = (frame.r.bpp / 8);

        cv::Mat input = cv::Mat(frame.r.h, frame.r.w, CV_MAKETYPE(CV_8U,numColorChan), frame.pixels.ptr());
        cv::Mat colorConv = cv::Mat(frame.r.h, frame.r.w, CV_8UC4, COLORCONV_BUFFER.ptr());

        k_assert(!COLORCONV_BUFFER.is_null(),
                 "Was asked to convert a frame's color depth, but the color conversion buffer "
                 "was null.");

        if (kc_output_pixel_format() == RGB_PIXELFORMAT_565)
        {
            conversionType = CV_BGR5652BGRA;
        }
        else if (kc_output_pixel_format() == RGB_PIXELFORMAT_555)
        {
            conversionType = CV_BGR5552BGRA;
        }
        else // The third pixel format we recognize is RGB_PIXELFORMAT_888; it should never need this conversion, as it arrives in BGRA.
        {
            // k_assert(0, "Was asked to scale a frame from an unknown pixel format.");
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

        frame.r.bpp = 32;
    #else
        k_assert(0, "Was asked to convert the frame to BGRA, but OpenCV had been disabled in the build. Can't do it.");
    #endif

    return;
}

// Takes the given image and scales it according to the scaler's current internal
// resolution settings. The scaled image is placed in the scaler's internal buffer,
// not in the source buffer.
//
void ks_scale_frame(captured_frame_s &frame)
{
    u8 *pixelData = frame.pixels.ptr();
    resolution_s outputRes = ks_output_resolution();

    // Verify that we have a workable frame.
    {
        if (kc_should_skip_next_frame())
        {
            DEBUG(("Skipping a frame, as requested."));
            goto done;
        }
        else if (frame.r.bpp != 16 && frame.r.bpp != 24 && frame.r.bpp != 32)
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
        else if (frame.r.bpp != kc_capture_color_depth())
        {
            NBENE(("Was asked to scale a frame whose bit depth (%u bits) differed from the expected (%u bits). Ignoring it.",
                   frame.r.bpp, kc_capture_color_depth()));
            goto done;
        }
        else if (frame.r.bpp > MAX_OUTPUT_BPP)
        {
            NBENE(("Was asked to scale a frame with a color depth (%u bits) higher than that allowed (%u bits). Ignoring it.",
                   frame.r.bpp, MAX_OUTPUT_BPP));
            goto done;
        }
        else if (frame.r.w < kc_hardware_min_capture_resolution().w ||
                 frame.r.h < kc_hardware_min_capture_resolution().h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) smaller than the minimum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, kc_hardware_min_capture_resolution().w, kc_hardware_min_capture_resolution().h));
            goto done;
        }
        else if (frame.r.w > kc_hardware_max_capture_resolution().w ||
                 frame.r.h > kc_hardware_max_capture_resolution().h)
        {
            NBENE(("Was asked to scale a frame with an input size (%u x %u) larger than the maximum allowed (%u x %u). Ignoring it.",
                   frame.r.w, frame.r.h, kc_hardware_max_capture_resolution().w, kc_hardware_max_capture_resolution().h));
            goto done;
        }
        else if (OUTPUT_BUFFER.is_null())
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

        pixelData = COLORCONV_BUFFER.ptr();
    }

    // Perform anti-tearing on the (color-converted) frame. If the user has turned
    // anti-tearing off, this will just return without doing anything.
    pixelData = kat_anti_tear(pixelData, frame.r);
    if (pixelData == nullptr)
    {
        goto done;
    }

    // Apply filtering, and scale the frame.
    {
        kf_apply_pre_filters(pixelData, frame.r);

        // If no need to scale, just copy the data over.
        if (frame.r.w == outputRes.w &&
            frame.r.h == outputRes.h)
        {
            memcpy(OUTPUT_BUFFER.ptr(), pixelData, OUTPUT_BUFFER.up_to(frame.r.w * frame.r.h * (OUTPUT_BIT_DEPTH / 8)));
        }
        else if (DOWNSCALE_FILTER == nullptr ||
                 UPSCALE_FILTER == nullptr)
        {
            NBENE(("Upscale or downscale filter is null. Refusing to scale."));

            outputRes = frame.r;
            memcpy(OUTPUT_BUFFER.ptr(), pixelData, OUTPUT_BUFFER.up_to(frame.r.w * frame.r.h * (OUTPUT_BIT_DEPTH / 8)));
        }
        else
        {
            const scaling_filter_s *f;

            if (kf_current_filter_set_scaler() != nullptr)
            {
                f = kf_current_filter_set_scaler();
            }
            else if (frame.r.w < outputRes.w ||
                     frame.r.h < outputRes.h)
            {
                f = UPSCALE_FILTER;
            }
            else
            {
                f = DOWNSCALE_FILTER;
            }

            f->scale(pixelData, frame.r, outputRes);
        }

        kf_apply_post_filters(OUTPUT_BUFFER.ptr(), outputRes);
    }

    LATEST_OUTPUT_SIZE = outputRes;

    kd_update_gui_filter_set_idx(kf_current_filter_set_idx());

    frame.processed = true;

    done:
    return;
}

void ks_set_output_resolution_override_enabled(const bool state)
{
    FORCE_BASE_RESOLUTION = state;
    kd_update_display_size();

    return;
}

void ks_set_output_pad_override_enabled(const bool state)
{
    FORCE_PADDING = state;
    kd_update_display_size();

    return;
}

void ks_set_output_pad_resolution(const resolution_s &r)
{
    PADDED_RESOLUTION = r;
    kd_update_display_size();

    return;
}

void ks_set_output_base_resolution(const resolution_s &r,
                                   const bool originatesFromUser)
{
    if (FORCE_BASE_RESOLUTION &&
        !originatesFromUser)
    {
        return;
    }

    BASE_RESOLUTION = r;
    kd_update_display_size();

    return;
}

real ks_output_scaling(void)
{
    return OUTPUT_SCALING;
}

void ks_set_output_scaling(const real s)
{
    OUTPUT_SCALING = s;
    kd_update_display_size();

    return;
}

void ks_set_output_scale_override_enabled(const bool state)
{
    FORCE_SCALING = state;

    kd_update_display_size();

    return;
}

void ks_set_output_aspect_ratio_override_enabled(const bool state)
{
    FORCE_ASPECT_RATIO = state;

    kd_update_display_size();

    return;
}

void ks_set_output_aspect_ratio(const u32 w, const u32 h)
{
    ASPECT_RATIO.w = w;
    ASPECT_RATIO.h = h;

    kd_update_display_size();

    return;
}

void ks_indicate_no_signal(void)
{
    ks_clear_scaler_output_buffer();

    return;
}

void ks_indicate_invalid_signal(void)
{
    ks_clear_scaler_output_buffer();

    return;
}

void ks_clear_scaler_output_buffer(void)
{
    k_assert(!OUTPUT_BUFFER.is_null(),
             "Can't access the output buffer: it was unexpectedly null.");

    memset(OUTPUT_BUFFER.ptr(), 0, OUTPUT_BUFFER.up_to(MAX_FRAME_SIZE));

    return;
}

const u8* ks_scaler_output_as_raw_ptr(void)
{
    return OUTPUT_BUFFER.ptr();
}

QImage ks_scaler_output_buffer_as_qimage(void)
{
    QImage img;

    if (OUTPUT_BUFFER.is_null())
    {
        DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
        img = QImage();
    }
    else
    {
        const resolution_s r = LATEST_OUTPUT_SIZE;
        img = QImage(OUTPUT_BUFFER.ptr(), r.w, r.h, QImage::Format_RGB32);
    }

    return img;
}

// Returns a list of GUI-displayable names of the scaling filters that're
// available.
//
QStringList ks_list_of_scaling_filter_names(void)
{
    QStringList names;

    for (uint i = 0; i < SCALING_FILTERS.size(); i++)
    {
        names += SCALING_FILTERS[i].name;
    }

    return names;
}

// Returns a scaling filter matching the given name.
//
const scaling_filter_s* ks_scaler_for_name_string(const QString &name)
{
    const scaling_filter_s *f = nullptr;

    k_assert(!SCALING_FILTERS.empty(),
             "Could find no scaling filters to search.");

    for (size_t i = 0; i < SCALING_FILTERS.size(); i++)
    {
        if (SCALING_FILTERS[i].name == name)
        {
            f = &SCALING_FILTERS[i];
            goto done;
        }
    }

    f = &SCALING_FILTERS.at(0);
    NBENE(("Was unable to find a scaler called '%s'. "
           "Defaulting to the first scaler on the list (%s).",
           name.toLatin1().constData(), f->name.toLatin1().constData()));

    done:
    return f;
}

const QString& ks_upscaling_filter_name(void)
{
    k_assert(UPSCALE_FILTER != nullptr,
             "Tried to get the name of a null upscale filter.");

    return UPSCALE_FILTER->name;
}

const QString& ks_downscaling_filter_name(void)
{
    k_assert(UPSCALE_FILTER != nullptr,
             "Tried to get the name of a null downscale filter.")

    return DOWNSCALE_FILTER->name;
}

void ks_set_upscaling_filter(const QString &name)
{
    UPSCALE_FILTER = ks_scaler_for_name_string(name);

    DEBUG(("Assigned '%s' as the upscaling filter.",
           UPSCALE_FILTER->name.toLatin1().constData()));

    return;
}

void ks_set_downscaling_filter(const QString &name)
{
    DOWNSCALE_FILTER = ks_scaler_for_name_string(name);

    DEBUG(("Assigned '%s' as the downscaling filter.",
           DOWNSCALE_FILTER->name.toLatin1().constData()));

    return;
}

resolution_s ks_scaler_output_aspect_ratio(void)
{
    return ASPECT_RATIO;
}

#ifdef VALIDATION_RUN
    const u8* ks_VALIDATION_raw_output_buffer_ptr(void)
    {
        return OUTPUT_BUFFER.ptr();
    }
#endif
