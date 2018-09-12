/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <QElapsedTimer>
#include <vector>
#include <map>
#include "qt/df_filters.h"
#include "display.h"
#include "capture.h"
#include "filter.h"
#include "common.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/core/core.hpp>
#endif

static bool FILTERING_ENABLED = false;

// Note: all filter functions expect the input pixels to be in 32-bit color.
static void filter_func_blur(FILTER_FUNC_PARAMS);
static void filter_func_uniquecount(FILTER_FUNC_PARAMS);
static void filter_func_unsharpmask(FILTER_FUNC_PARAMS);
static void filter_func_deltahistogram(FILTER_FUNC_PARAMS);
static void filter_func_decimate(FILTER_FUNC_PARAMS);
static void filter_func_denoise(FILTER_FUNC_PARAMS);
static void filter_func_sharpen(FILTER_FUNC_PARAMS);
static void filter_func_median(FILTER_FUNC_PARAMS);

// Establish a list of all the filters which the user can apply. For each filter,
// assign a pointer to the function which applies that filter to a given array of
// pixels, and a point to a user-facing GUI dialog with which the user can adjust
// the filter's parameters.
static const std::map<QString, std::pair<filter_function_t, filter_dlg_s*>> FILTERS =
               {{"Delta Histogram",  {filter_func_deltahistogram,  []{ static filter_dlg_deltahistogram_s f; return &f; }()}},
                {"Unique Count",     {filter_func_uniquecount,     []{ static filter_dlg_uniquecount_s f; return &f;    }()}},
                {"Unsharp Mask",     {filter_func_unsharpmask,     []{ static filter_dlg_unsharpmask_s f; return &f;    }()}},
                {"Blur",             {filter_func_blur,            []{ static filter_dlg_blur_s f; return &f;           }()}},
                {"Decimate",         {filter_func_decimate,        []{ static filter_dlg_decimate_s f; return &f;       }()}},
                {"Denoise",          {filter_func_denoise,         []{ static filter_dlg_denoise_s f; return &f;        }()}},
                {"Sharpen",          {filter_func_sharpen,         []{ static filter_dlg_sharpen_s f; return &f;        }()}},
                {"Median",           {filter_func_median,          []{ static filter_dlg_median_s f; return &f;         }()}}};

// Each filter complex is a collection of one or more filters, which together will
// be applied to incoming frames if that filter complex is active.
static std::vector<filter_complex_s> FILTER_COMPLEXES;
static i32 CUR_FILTER_COMPLEX_IDX = -1;     // Index in the filter complex list of the currently-active complex.

// All filters expect 32-bit color, i.e. 4 channels.
const uint NUM_COLOR_CHAN = (32 / 8);

// Returns a list of the names of all of the user-facing filters.
//
QStringList kf_filter_name_list(void)
{
    QStringList l;

    for (auto &f: FILTERS)
    {
        l << f.first;
    }

    return l;
}

void kf_clear_filters(void)
{
    FILTER_COMPLEXES.clear();

    return;
}

// Returns the filter complex matching the given input and output resolution.
//
const filter_complex_s* kf_filter_complex_for_resolutions(const resolution_s &in,
                                                          const resolution_s &out)
{
    const filter_complex_s *f = nullptr;

    for (size_t i = 0; i < FILTER_COMPLEXES.size(); i++)
    {
        if (FILTER_COMPLEXES[i].inRes.w == in.w &&
            FILTER_COMPLEXES[i].inRes.h == in.h &&
            FILTER_COMPLEXES[i].outRes.w == out.w &&
            FILTER_COMPLEXES[i].outRes.h == out.h)
        {
            f = &FILTER_COMPLEXES[i];

            break;
        }
    }

    k_assert(f != nullptr,
             "Was unable to return a valid filter complex for the given resolutions.");

    return f;
}

const scaling_filter_s* kf_current_filter_complex_scaler(void)
{
    if (CUR_FILTER_COMPLEX_IDX < 0 ||
        CUR_FILTER_COMPLEX_IDX >= (int)FILTER_COMPLEXES.size() ||
        !FILTERING_ENABLED ||
        !FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX].isEnabled)
    {
        return nullptr;
    }

    return FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX].scaler;
}

void kf_release_filters(void)
{
    DEBUG(("Releasing custom filtering."));

    /// Doesn't have anything to do at the moment.

    return;
}

const std::vector<filter_complex_s>& kf_filter_complexes(void)
{
    return FILTER_COMPLEXES;
}

// Add or update the given filter complex.
//
void kf_update_filter_complex(const filter_complex_s &f)
{
    // Find this filter on out list and update it.
    for (auto &complex: FILTER_COMPLEXES)
    {
        if ((complex.inRes.w == f.inRes.w) && (complex.inRes.h == f.inRes.h) &&
            (complex.outRes.w == f.outRes.w) && (complex.outRes.h == f.outRes.h))
        {
            complex = f;
            goto done;
        }
    }

    // If we didn't find the filter on the list, we'll add it there.
    FILTER_COMPLEXES.push_back(f);
    kf_activate_filter_complex_for(kc_input_resolution(), ks_output_resolution());

    done:
    return;
}

// Looks to find whether we know of a filter complex matching the given resolutions;
// and if we do, set it as the currently-active one.
//
void kf_activate_filter_complex_for(const resolution_s &inR, const resolution_s &outR)
{
    for (size_t i = 0; i < FILTER_COMPLEXES.size(); i++)
    {
        const auto &complex = FILTER_COMPLEXES[i];

        if ((complex.inRes.w == inR.w) && (complex.inRes.h == inR.h) &&
            (complex.outRes.w == outR.w) && (complex.outRes.h == outR.h))
        {
            CUR_FILTER_COMPLEX_IDX = i;
            goto done;
        }
    }

    // We couldn't find a filter for this resolution, so mark not to use one.
    CUR_FILTER_COMPLEX_IDX = -1;

    done:
    //DEBUG(("Set the current filter complex index to %d.", CUR_FILTER_IDX));
    return;
}

// Counts the number of unique frames per second, i.e. frames in which the pixels
// change between frames by less than a set threshold (which is to account for
// analog capture artefacts).
//
static void filter_func_uniquecount(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    static heap_bytes_s<u8> prevPixels(MAX_FRAME_SIZE, "Unique count filter buffer");
    const u8 threshold = params[filter_dlg_uniquecount_s::OFFS_THRESHOLD];
    static u32 uniqueFrames = 0;
    static u32 uniquePerSecond = 0;
    static QElapsedTimer tim;
    if (!tim.isValid())
    {
        tim.start();
    }

    for (u32 i = 0; i < (r->w * r->h); i++)
    {
        const u32 idx = i * NUM_COLOR_CHAN;

        if (abs(pixels[idx + 0] - prevPixels[idx + 0]) > threshold ||
            abs(pixels[idx + 1] - prevPixels[idx + 1]) > threshold ||
            abs(pixels[idx + 2] - prevPixels[idx + 2]) > threshold)
        {
            uniqueFrames++;

            break;
        }
    }

    memcpy(prevPixels.ptr(), pixels, prevPixels.up_to(r->w * r->h * (r->bpp / 8)));

    // Once per second.
    if (tim.elapsed() > 1000)
    {
        uniquePerSecond = round(uniqueFrames / (tim.elapsed() / 1000.0));
        uniqueFrames = 0;
        tim.restart();
    }

    std::string str = std::to_string(uniquePerSecond);
    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::putText(output, str, cv::Point(0, 24),
                cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 230, 255), 2);
#endif

    return;
}

// Reduce image noise.
//
static void filter_func_denoise(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    const u8 type = filter_dlg_denoise_s::FILTER_TYPE_TEMPORAL;

    if (type == filter_dlg_denoise_s::FILTER_TYPE_SPATIAL)
    {
#if 0 /// TODO. Runs real slow.
        static u8 *const denoiseBuffer = (u8*)malloc(PAGE_SIZE);//TEMP_BUFFER + PAGE_DENOISE;
        const int templateWindowSize = 3;
        const int searchWindowSize = 3;
        const int hLuminance = 1;
        const int hColor = 4;

        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::Mat denoised = cv::Mat(r->h, r->w, CV_8UC1, denoiseBuffer);

        // All channels.
        cv::cvtColor(output, denoised, CV_BGRA2BGR);
        cv::fastNlMeansDenoisingColored(denoised, denoised,
                                        templateWindowSize,
                                        searchWindowSize,
                                        hLuminance,
                                        hColor);
        cv::cvtColor(denoised, output, CV_BGR2BGRA);
#endif
    }
    else if (type == filter_dlg_denoise_s::FILTER_TYPE_TEMPORAL)
    {
        // Denoise temporally, i.e. require pixels to change by > threshold between
        // frames before they're allowed through.

        const u8 threshold = params[filter_dlg_denoise_s::OFFS_THRESHOLD];
        static heap_bytes_s<u8> prevPixels(MAX_FRAME_SIZE, "Denoising filter buffer");

        for (uint i = 0; i < (r->h * r->w); i++)
        {
            const u32 idx = i * NUM_COLOR_CHAN;

            if ((abs(pixels[idx + 0] - prevPixels[idx + 0]) > threshold) ||
                (abs(pixels[idx + 1] - prevPixels[idx + 1]) > threshold) ||
                (abs(pixels[idx + 2] - prevPixels[idx + 2]) > threshold))
            {
                prevPixels[idx + 0] = pixels[idx + 0];
                prevPixels[idx + 1] = pixels[idx + 1];
                prevPixels[idx + 2] = pixels[idx + 2];
            }
            else
            {
                pixels[idx + 0] = prevPixels[idx + 0];
                pixels[idx + 1] = prevPixels[idx + 1];
                pixels[idx + 2] = prevPixels[idx + 2];
            }
        }
    }
    else
    {
        NBENE(("Unknown filter type for denoising. Skipping it."));
    }
#endif

    return;
}

// Draws a histogram by color value of the number of pixels changed between frames.
//
static void filter_func_deltahistogram(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    static heap_bytes_s<u8> prevFramePixels(MAX_FRAME_SIZE, "Delta histogram buffer");

    const uint numBins = 512;

    // For each RGB channel, count into bins how many times a particular delta
    // between pixels in the previous frame and this one occurred.
    u32 bl[numBins] = {0};
    u32 gr[numBins] = {0};
    u32 re[numBins] = {0};
    for (u32 i = 0; i < (r->w * r->h); i++)
    {
        const u32 idx = i * NUM_COLOR_CHAN;
        const u32 deltaBlue = (pixels[idx + 0] - prevFramePixels[idx + 0]) + 255;
        const u32 deltaGreen = (pixels[idx + 1] - prevFramePixels[idx + 1]) + 255;
        const u32 deltaRed = (pixels[idx + 2] - prevFramePixels[idx + 2]) + 255;

        k_assert(deltaBlue < numBins, "");
        k_assert(deltaGreen < numBins, "");
        k_assert(deltaRed < numBins, "");

        bl[deltaBlue]++;
        gr[deltaGreen]++;
        re[deltaRed]++;
    }

    // Draw the bins into the frame as a graph.
    const uint maxval = r->w * r->h;
    for (u32 i = 0; i < numBins; i++)
    {
        real xskip = (r->w / (real)numBins);
        u32 x = xskip * i;

        if (x >= r->w)
        {
            x = (r->w - 1);
        }
        if ((x + xskip) > r->w)
        {
            xskip = r->w - x;
        }

        const u32 yb = r->h - ((r->h / 256.0) * ((256.0 / maxval) * bl[i]));
        const u32 yg = r->h - ((r->h / 256.0) * ((256.0 / maxval) * gr[i]));
        const u32 yr = r->h - ((r->h / 256.0) * ((256.0 / maxval) * re[i]));

        for (u32 y = yb; y < r->h; y++)
        {
            for (u32 xs = x; xs < (x + xskip); xs++)
            {
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 0] = 255;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 1] = 0;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 2] = 0;
            }
        }

        for (u32 y = yg; y < r->h; y++)
        {
            for (u32 xs = x; xs < (x + xskip); xs++)
            {
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 0] = 0;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 1] = 255;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 2] = 0;
            }
        }

        for (u32 y = yr; y < r->h; y++)
        {
            for (u32 xs = x; xs < (x + xskip); xs++)
            {
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 0] = 0;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 1] = 0;
                pixels[(xs + y * r->w) * NUM_COLOR_CHAN + 2] = 255;
            }
        }
    }

    // Temp buf #1 will now contain the original frame data we received, before
    // we made changes to it.
    memcpy(prevFramePixels.ptr(), pixels, prevFramePixels.up_to(r->w * r->h * (r->bpp / 8)));
#endif

    return;
}

static void filter_func_unsharpmask(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    static heap_bytes_s<u8> TMP_BUF(MAX_FRAME_SIZE, "Unsharp mask buffer");
    const real str = params[filter_dlg_unsharpmask_s::OFFS_STRENGTH] / 100.0;
    const real rad = params[filter_dlg_unsharpmask_s::OFFS_RADIUS] / 10.0;

    cv::Mat tmp = cv::Mat(r->h, r->w, CV_8UC4, TMP_BUF.ptr());
    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
    cv::addWeighted(output, 1 + str, tmp, -str, 0, output);
#endif

    return;
}

static void filter_func_sharpen(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    float kernel[] = { 0, -1,  0,
                      -1,  5, -1,
                       0, -1,  0};

    cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::filter2D(output, output, -1, ker);
#endif

    return;
}

// Pixelates.
//
static void filter_func_decimate(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    const u8 factor = params[filter_dlg_decimate_s::OFFS_FACTOR];
    const u8 type = params[filter_dlg_decimate_s::OFFS_TYPE];

    for (u32 y = 0; y < r->h; y += factor)
    {
        for (u32 x = 0; x < r->w; x += factor)
        {
            int ar = 0, ag = 0, ab = 0;

            if (type == filter_dlg_decimate_s::FILTER_TYPE_AVERAGE)
            {
                for (int yd = 0; yd < factor; yd++)
                {
                    for (int xd = 0; xd < factor; xd++)
                    {
                        const u32 idx = ((x + xd) + (y + yd) * r->w) * NUM_COLOR_CHAN;

                        ab += pixels[idx + 0];
                        ag += pixels[idx + 1];
                        ar += pixels[idx + 2];
                    }
                }
                ar /= (factor * factor);
                ag /= (factor * factor);
                ab /= (factor * factor);
            }
            else if (type == filter_dlg_decimate_s::FILTER_TYPE_NEAREST)
            {
                const u32 idx = (x + y * r->w) * NUM_COLOR_CHAN;

                ab = pixels[idx + 0];
                ag = pixels[idx + 1];
                ar = pixels[idx + 2];
            }

            for (int yd = 0; yd < factor; yd++)
            {
                for (int xd = 0; xd < factor; xd++)
                {
                    const u32 idx = ((x + xd) + (y + yd) * r->w) * NUM_COLOR_CHAN;

                    pixels[idx + 0] = ab;
                    pixels[idx + 1] = ag;
                    pixels[idx + 2] = ar;
                }
            }
        }
    }
#endif

    return;
}

static void filter_func_median(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    const u8 kernelS = params[filter_dlg_median_s::OFF_KERNEL_SIZE];

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::medianBlur(output, output, kernelS);
#endif

    return;
}

static void filter_func_blur(FILTER_FUNC_PARAMS)
{
    k_assert(r->bpp == 32, "This filter expects 32-bit source color.");
    if (pixels == nullptr || params == nullptr || r == nullptr)
    {
        return;
    }

#ifdef USE_OPENCV
    const real kernelS = (params[filter_dlg_blur_s::OFF_KERNEL_SIZE] / 10.0);

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);

    if (params[filter_dlg_blur_s::OFF_TYPE] == filter_dlg_blur_s::FILTER_TYPE_GAUSSIAN)
    {
        cv::GaussianBlur(output, output, cv::Size(0, 0), kernelS);
    }
    else
    {
        const u8 kernelW = ((int(kernelS) * 2) + 1);
        cv::blur(output, output, cv::Size(kernelW, kernelW));
    }
#endif

    return;
}

bool kf_named_filter_exists(const QString &name)
{
    // If we reach the end of the functions list before finding one by the given
    // name, the entry doesn't exist.
    const bool exists = (FILTERS.find(name) != FILTERS.end());

    return exists;
}

const filter_dlg_s* kf_filter_dialog_for_name(const QString &name)
{
    return FILTERS.at(name).second;
}

filter_function_t kf_filter_function_ptr_for_name(const QString &name)
{
    return FILTERS.at(name).first;
}

// Marks the filter complex of the given in/out resolution as enabled/disabled.
//
void kf_set_filter_complex_enabled(const bool enabled,
                                   const resolution_s &inRes, const resolution_s &outRes)
{
    for (auto &complex: FILTER_COMPLEXES)
    {
        if ((complex.inRes.w == inRes.w) && (complex.inRes.h == inRes.h) &&
            (complex.outRes.w == outRes.w) && (complex.outRes.h == outRes.h))
        {
            complex.isEnabled = enabled;

            return;
        }
    }

    NBENE(("Failed to find a filter complex with the given resolutions to enable/disable. Ignoring the request."));

    return;
}

void kf_set_filtering_enabled(const bool enabled)
{
    FILTERING_ENABLED = enabled;

    return;
}

static void apply_filter(const QString &name, FILTER_FUNC_PARAMS)
{
    filter_function_t applyFn = kf_filter_function_ptr_for_name(name);

    applyFn(pixels, r, params);

    return;
}

void kf_apply_pre_filters(u8 *const pixels, const resolution_s &r)
{
    // If there are no filters to apply, quit.
    if (!FILTERING_ENABLED ||
        (CUR_FILTER_COMPLEX_IDX < 0) ||
        (CUR_FILTER_COMPLEX_IDX >= (int)FILTER_COMPLEXES.size()) ||
        !FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX].isEnabled)
    {
        return;
    }

    k_assert(r.bpp == 32,
             "The pre-filterer was given a frame with an unexpected bit depth.");

    const filter_complex_s *const filterComplex = &FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX];
    for (auto &f: filterComplex->preFilters)
    {
        apply_filter(f.name, pixels, &r, f.data);
    }

    return;
}

/// TODO. Code duplication - merge this with the function to apply pre-filters.
void kf_apply_post_filters(u8 *const pixels, const resolution_s &r)
{
    // If there are no filters to apply, quit.
    if (!FILTERING_ENABLED ||
        (CUR_FILTER_COMPLEX_IDX < 0) ||
        (CUR_FILTER_COMPLEX_IDX >= (int)FILTER_COMPLEXES.size()) ||
        !FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX].isEnabled)
    {
        return;
    }

    k_assert(r.bpp == 32,
             "The post-filterer was given a frame with an unexpected bit depth.");

    const filter_complex_s *const filterComplex = &FILTER_COMPLEXES[CUR_FILTER_COMPLEX_IDX];
    for (auto &f: filterComplex->postFilters)
    {
        apply_filter(f.name, pixels, &r, f.data);
    }

    return;
}

void kf_initialize_filters(void)
{
    DEBUG(("Initializing custom filtering."));

    // Make sure the filter dialog names match the names of their corresponding
    // filters.
    for (auto f: FILTERS)
    {
        k_assert(f.second.first == kf_filter_function_ptr_for_name(f.second.second->name()),
                 "Found a filter whose screen name didn't match its base name.");
    }

    return;
}
