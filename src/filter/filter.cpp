    /*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <vector>
#include <map>
#include <ctime>
#include "../display/qt/df_filters.h"
#include "../display/display.h"
#include "../capture/capture.h"
#include "../common/globals.h"
#include "filter.h"

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
static void filter_func_crop(FILTER_FUNC_PARAMS);
static void filter_func_flip(FILTER_FUNC_PARAMS);
static void filter_func_rotate(FILTER_FUNC_PARAMS);

// Invoke this macro at the start of each filter_func_*() function, to verify
// that the parameters passed are valid to operate on.
#define VALIDATE_FILTER_INPUT  k_assert(r->bpp == 32, "This filter expects 32-bit source color.");\
                               if (pixels == nullptr || params == nullptr || r == nullptr) return;

// Establish a list of all the filters which the user can apply. For each filter,
// assign a pointer to the function which applies that filter to a given array of
// pixels, and a point to a user-facing GUI dialog with which the user can adjust
// the filter's parameters.
static const std::map<std::string, std::pair<filter_function_t, filter_dlg_s*>> FILTERS =
               {{"Delta Histogram",  {filter_func_deltahistogram,  []{ static filter_dlg_deltahistogram_s f; return &f; }()}},
                {"Unique Count",     {filter_func_uniquecount,     []{ static filter_dlg_uniquecount_s f; return &f;    }()}},
                {"Unsharp Mask",     {filter_func_unsharpmask,     []{ static filter_dlg_unsharpmask_s f; return &f;    }()}},
                {"Blur",             {filter_func_blur,            []{ static filter_dlg_blur_s f; return &f;           }()}},
                {"Decimate",         {filter_func_decimate,        []{ static filter_dlg_decimate_s f; return &f;       }()}},
                {"Denoise",          {filter_func_denoise,         []{ static filter_dlg_denoise_s f; return &f;        }()}},
                {"Sharpen",          {filter_func_sharpen,         []{ static filter_dlg_sharpen_s f; return &f;        }()}},
                {"Median",           {filter_func_median,          []{ static filter_dlg_median_s f; return &f;         }()}},
                {"Crop",             {filter_func_crop,            []{ static filter_dlg_crop_s f; return &f;           }()}},
                {"Flip",             {filter_func_flip,            []{ static filter_dlg_flip_s f; return &f;           }()}},
                {"Rotate",           {filter_func_rotate,          []{ static filter_dlg_rotate_s f; return &f;         }()}}};

static std::vector<filter_set_s*> FILTER_SETS;

// The index in the list of filter sets of the set which was most recently used.
// Generally, this will be the filter set that matches the current input/output
// resolution.
static int MOST_RECENT_FILTER_SET_IDX = -1;

// All filters expect 32-bit color, i.e. 4 channels.
const uint NUM_COLOR_CHAN = (32 / 8);

// Returns a list of the names of all of the user-facing filters.
//
std::vector<std::string> kf_filter_name_list(void)
{
    std::vector<std::string> list;
    for (auto &f: FILTERS)
    {
        list.push_back(f.first);
    }

    return list;
}

static void clear_filter_sets_list(void)
{
    for (auto set: FILTER_SETS)
    {
        delete set;
    }

    FILTER_SETS.clear();
    MOST_RECENT_FILTER_SET_IDX = -1;

    return;
}

void kf_clear_filters(void)
{
    clear_filter_sets_list();

    return;
}

void kf_set_filter_set_enabled(const uint idx, const bool isEnabled)
{
    FILTER_SETS.at(idx)->isEnabled = isEnabled;
    return;
}

const scaling_filter_s* kf_current_filter_set_scaler(void)
{
    return ((kf_current_filter_set() == nullptr)? nullptr : kf_current_filter_set()->scaler);
}

void kf_release_filters(void)
{
    DEBUG(("Releasing custom filtering."));

    clear_filter_sets_list();

    return;
}

void kf_add_filter_set(filter_set_s *const newSet)
{
    FILTER_SETS.push_back(newSet);

    return;
}

void kf_filter_set_swap_upward(const uint idx)
{
    if (idx <= 0) return;
    std::swap(FILTER_SETS.at(idx), FILTER_SETS.at(idx-1));
}

void kf_filter_set_swap_downward(const uint idx)
{
    if (idx >= (FILTER_SETS.size() - 1)) return;
    std::swap(FILTER_SETS.at(idx), FILTER_SETS.at(idx+1));
}

void kf_remove_filter_set(const uint idx)
{
    k_assert((idx <= FILTER_SETS.size()), "Attempting to remove a filter set out of bounds.");

    FILTER_SETS.erase(FILTER_SETS.begin() + idx);

    return;
}

const std::vector<filter_set_s*>& kf_filter_sets(void)
{
    return FILTER_SETS;
}

// Counts the number of unique frames per second, i.e. frames in which the pixels
// change between frames by less than a set threshold (which is to account for
// analog capture artefacts).
//
static void filter_func_uniquecount(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    static heap_bytes_s<u8> prevPixels(MAX_FRAME_SIZE, "Unique count filter buffer");

    const u8 threshold = params[filter_dlg_uniquecount_s::OFFS_THRESHOLD];
    const u8 corner = params[filter_dlg_uniquecount_s::OFFS_CORNER];

    static u32 uniqueFramesProcessed = 0;
    static u32 uniqueFramesPerSecond = 0;
    static time_t timer = time(NULL);

    for (u32 i = 0; i < (r->w * r->h); i++)
    {
        const u32 idx = i * NUM_COLOR_CHAN;

        if (abs(pixels[idx + 0] - prevPixels[idx + 0]) > threshold ||
            abs(pixels[idx + 1] - prevPixels[idx + 1]) > threshold ||
            abs(pixels[idx + 2] - prevPixels[idx + 2]) > threshold)
        {
            uniqueFramesProcessed++;

            break;
        }
    }

    memcpy(prevPixels.ptr(), pixels, prevPixels.up_to(r->w * r->h * (r->bpp / 8)));

    const double secsElapsed = difftime(time(NULL), timer);
    if (secsElapsed >= 1)
    {
        uniqueFramesPerSecond = round(uniqueFramesProcessed / secsElapsed);
        uniqueFramesProcessed = 0;
        timer = time(NULL);
    }

    // Draw the counter into the frame.
    {
        std::string counterString = std::to_string(uniqueFramesPerSecond);

        const auto cornerPos = [corner, r, counterString]()->cv::Point
        {
            switch (corner)
            {
                // Top right.
                case 1: return cv::Point(r->w - (counterString.length() * 24), 24);

                // Bottom right.
                case 2: return cv::Point(r->w - (counterString.length() * 24), r->h - 12);

                // Bottom left.
                case 3: return cv::Point(0, r->h - 12);

                default: return cv::Point(0, 24);
            }
        }();

        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::putText(output, counterString, cornerPos, cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 230, 255), 2);
    }
#endif

    return;
}

// Reduce image noise.
//
static void filter_func_denoise(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

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
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    static heap_bytes_s<u8> prevFramePixels(MAX_FRAME_SIZE, "Delta histogram buffer");

    const uint numBins = 512;

    // For each RGB channel, count into bins how many times a particular delta
    // between pixels in the previous frame and this one occurred.
    uint bl[numBins] = {0};
    uint gr[numBins] = {0};
    uint re[numBins] = {0};
    for (uint i = 0; i < (r->w * r->h); i++)
    {
        const uint idx = i * NUM_COLOR_CHAN;
        const uint deltaBlue = (pixels[idx + 0] - prevFramePixels[idx + 0]) + 255;
        const uint deltaGreen = (pixels[idx + 1] - prevFramePixels[idx + 1]) + 255;
        const uint deltaRed = (pixels[idx + 2] - prevFramePixels[idx + 2]) + 255;

        k_assert(deltaBlue < numBins, "");
        k_assert(deltaGreen < numBins, "");
        k_assert(deltaRed < numBins, "");

        bl[deltaBlue]++;
        gr[deltaGreen]++;
        re[deltaRed]++;
    }

    // Draw the bins into the frame as a line graph.
    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    for (uint i = 1; i < numBins; i++)
    {
        const uint maxval = r->w * r->h;
        real xskip = (r->w / (real)numBins);

        uint x2 = xskip * i;
        uint x1 = x2-xskip;

        const uint y1b = r->h - ((r->h / 256.0) * ((256.0 / maxval) * bl[i-1]));
        const uint y2b = r->h - ((r->h / 256.0) * ((256.0 / maxval) * bl[i]));

        const uint y1g = r->h - ((r->h / 256.0) * ((256.0 / maxval) * gr[i-1]));
        const uint y2g = r->h - ((r->h / 256.0) * ((256.0 / maxval) * gr[i]));

        const uint y1r = r->h - ((r->h / 256.0) * ((256.0 / maxval) * re[i-1]));
        const uint y2r = r->h - ((r->h / 256.0) * ((256.0 / maxval) * re[i]));

        cv::line(output, cv::Point(x1, y1b), cv::Point(x2, y2b), cv::Scalar(255, 0, 0), 2, CV_AA);
        cv::line(output, cv::Point(x1, y1g), cv::Point(x2, y2g), cv::Scalar(0, 255, 0), 2, CV_AA);
        cv::line(output, cv::Point(x1, y1r), cv::Point(x2, y2r), cv::Scalar(0, 0, 255), 2, CV_AA);
    }

    memcpy(prevFramePixels.ptr(), pixels, prevFramePixels.up_to(r->w * r->h * (r->bpp / 8)));
#endif

    return;
}

static void filter_func_unsharpmask(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

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
    VALIDATE_FILTER_INPUT

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
    VALIDATE_FILTER_INPUT

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

// Takes a subregion of the frame and either scales it up to fill the whole frame or
// fills its surroundings with black.
//
static void filter_func_crop(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    uint x = *(u16*)&(params[filter_dlg_crop_s::OFFS_X]);
    uint y = *(u16*)&(params[filter_dlg_crop_s::OFFS_Y]);
    uint w = *(u16*)&(params[filter_dlg_crop_s::OFFS_WIDTH]);
    uint h = *(u16*)&(params[filter_dlg_crop_s::OFFS_HEIGHT]);

    #ifdef USE_OPENCV
        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        int scaler = -1;
        switch (params[filter_dlg_crop_s::OFFS_SCALER])
        {
            case 0: scaler = cv::INTER_LINEAR; break;
            case 1: scaler = cv::INTER_NEAREST; break;
            case 2: scaler = -1 /*Don't scale.*/; break;
            default: k_assert(0, "Unknown scaler type for the crop filter."); break;
        }

        if (((x + w) > r->w) || ((y + h) > r->h))
        {
            /// TODO: Signal a user-facing but non-obtrusive message about the crop
            /// params being invalid.
        }
        else
        {
            cv::Mat cropped = output(cv::Rect(x, y, w, h)).clone();

            // If the user doesn't want scaling, just append some black borders around the
            // cropping. Otherwise, stretch the cropped region to fill the entire frame.
            if (scaler < 0) cv::copyMakeBorder(cropped, output, y, (r->h - (h + y)), x, (r->w - (w + x)), cv::BORDER_CONSTANT, 0);
            else cv::resize(cropped, output, output.size(), 0, 0, scaler);
        }
    #endif

    return;
}

// Flips the frame horizontally and/or vertically.
//
static void filter_func_flip(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_FRAME_SIZE, "Flip filter scratch buffer");

    // 0 = vertical, 1 = horizontal, -1 = both.
    const uint axis = ((params[filter_dlg_flip_s::OFFS_AXIS] == 2)? -1 : params[filter_dlg_flip_s::OFFS_AXIS]);

    #ifdef USE_OPENCV
        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::Mat temp = cv::Mat(r->h, r->w, CV_8UC4, scratch.ptr());

        cv::flip(output, temp, axis);
        temp.copyTo(output);
    #endif

    return;
}

static void filter_func_rotate(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_FRAME_SIZE, "Rotate filter scratch buffer");

    const double angle = (*(i16*)&(params[filter_dlg_rotate_s::OFFS_ROT]) / 10.0);
    const double scale = (*(i16*)&(params[filter_dlg_rotate_s::OFFS_SCALE]) / 100.0);

    #ifdef USE_OPENCV
        cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::Mat temp = cv::Mat(r->h, r->w, CV_8UC4, scratch.ptr());

        cv::Mat transf = cv::getRotationMatrix2D(cv::Point2d((r->w / 2), (r->h / 2)), -angle, scale);
        cv::warpAffine(output, temp, transf, cv::Size(r->w, r->h));
        temp.copyTo(output);
    #endif

    return;
}

static void filter_func_median(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 kernelS = params[filter_dlg_median_s::OFF_KERNEL_SIZE];

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::medianBlur(output, output, kernelS);
#endif

    return;
}

static void filter_func_blur(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

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

bool kf_named_filter_exists(const std::string &name)
{
    // If we reach the end of the functions list before finding one by the given
    // name, the entry doesn't exist.
    const bool exists = (FILTERS.find(name) != FILTERS.end());

    return exists;
}

const filter_dlg_s* kf_filter_dialog_for_name(const std::string &name)
{
    return FILTERS.at(name).second;
}

// Returns a pointer to the filter function that corresponds to the given filter
// name string.
//
filter_function_t kf_filter_function_ptr_for_name(const std::string &name)
{
    return FILTERS.at(name).first;
}

void kf_set_filtering_enabled(const bool enabled)
{
    FILTERING_ENABLED = enabled;

    return;
}

static void apply_filter(const std::string &name, FILTER_FUNC_PARAMS)
{
    filter_function_t applyFn = kf_filter_function_ptr_for_name(name);

    applyFn(pixels, r, params);

    return;
}

// Find by how many pixels the given image is out of alignment with the
// edges of the capture screen vertically and horizontally, by counting how
// many vertical/horizontal columns/rows at the edges of the image contain
// nothing but black pixels. This is an estimate and not necessarily accurate -
// depending on the image, it may be grossly inaccurate. The vector returned
// contains as two components the image's alignment offset on the vertical
// and horizontal axes.
//
// This will not work if the image is fully contained within the capture screen -
// at least one edge of the image is expected to fall outside of the screen.
std::vector<int> kf_find_capture_alignment(u8 *const pixels, const resolution_s &r)
{
    k_assert((r.bpp == 32), "Expected 32-bit pixel data for finding image alignment.");

    // The level above which we consider a pixel's color value to be not part
    // of the black background.
    const int threshold = 50;

    const auto find_horizontal = [r, pixels, threshold](int x, int limit, int direction)->uint
    {
        uint steps = 0;
        while (x != limit)
        {
            for (uint y = 0; y < r.h; y++)
            {
                // If any pixel on this vertical column is above the threshold,
                // consider the column to be part of the image rather than of
                // the background.
                const uint idx = ((x + y * r.w) * 4);
                if (pixels[idx + 0] > threshold ||
                    pixels[idx + 1] > threshold ||
                    pixels[idx + 2] > threshold)
                {
                    goto done;
                }
            }

            steps++;
            x += direction;
        }

        done:
        return steps;
    };

    const auto find_vertical = [r, pixels, threshold](int y, int limit, int direction)->uint
    {
        uint steps = 0;
        while (y != limit)
        {
            for (uint x = 0; x < r.w; x++)
            {
                const uint idx = ((x + y * r.w) * 4);
                if (pixels[idx + 0] > threshold ||
                    pixels[idx + 1] > threshold ||
                    pixels[idx + 2] > threshold)
                {
                    goto done;
                }
            }

            steps++;
            y += direction;
        }

        done:
        return steps;
    };

    int left = find_horizontal(0, r.w-1, +1);
    int right = find_horizontal(r.w-1, 0, -1);
    int top = find_vertical(0, r.h-1, +1);
    int bottom = find_vertical(r.h-1, 0, -1);

    return {(left? left : -right),
            (top? top : -bottom)};
}

uint kf_current_filter_set_idx(void)
{
    return MOST_RECENT_FILTER_SET_IDX;
}

// Returns a pointer to the filter set which the current input/output resolutions
// activate.
const filter_set_s* kf_current_filter_set(void)
{
    const resolution_s inputRes = kc_hardware().status.capture_resolution();
    const resolution_s outputRes = ks_output_resolution();

    // Find the first filter set in our list of filter sets that fits the given
    // input and/or output resolution(s). If non are found, we return null.
    for (size_t i = 0; i < FILTER_SETS.size(); i++)
    {
        MOST_RECENT_FILTER_SET_IDX = i;

        if (!FILTER_SETS.at(i)->isEnabled) continue;

        if (FILTER_SETS.at(i)->activation & filter_set_s::activation_e::all)
        {
            return FILTER_SETS.at(i);
        }
        else if ((FILTER_SETS.at(i)->activation & filter_set_s::activation_e::in) &&
                 (FILTER_SETS.at(i)->activation & filter_set_s::activation_e::out))
        {
            if (inputRes.w == FILTER_SETS.at(i)->inRes.w &&
                inputRes.h == FILTER_SETS.at(i)->inRes.h &&
                outputRes.w == FILTER_SETS.at(i)->outRes.w &&
                outputRes.h == FILTER_SETS.at(i)->outRes.h)
            {
                return FILTER_SETS.at(i);
            }
        }
        else if (FILTER_SETS.at(i)->activation & filter_set_s::activation_e::in)
        {
            if (inputRes.w == FILTER_SETS.at(i)->inRes.w &&
                inputRes.h == FILTER_SETS.at(i)->inRes.h)
            {
                return FILTER_SETS.at(i);
            }
        }
    }

    MOST_RECENT_FILTER_SET_IDX = -1;
    return nullptr;
}

void kf_apply_pre_filters(u8 *const pixels, const resolution_s &r)
{
    k_assert(r.bpp == 32,
             "The pre-filterer was given a frame with an unexpected bit depth.");

    if (!FILTERING_ENABLED) return;

    const filter_set_s *const filterSet = kf_current_filter_set();
    if (filterSet == nullptr) return;

    for (auto &f: filterSet->preFilters)
    {
        apply_filter(f.name, pixels, &r, f.data);
    }

    return;
}

/// TODO. Code duplication - merge this with the function to apply pre-filters.
void kf_apply_post_filters(u8 *const pixels, const resolution_s &r)
{
    k_assert(r.bpp == 32,
             "The post-filterer was given a frame with an unexpected bit depth.");

    if (!FILTERING_ENABLED) return;

    const filter_set_s *const filterSet = kf_current_filter_set();
    if (filterSet == nullptr) return;

    for (auto &f: filterSet->postFilters)
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
