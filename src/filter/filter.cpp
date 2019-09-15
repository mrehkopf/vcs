    /*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include <unordered_map>
#include <cstring>
#include <vector>
#include <ctime>
#include <cmath>
#include <map>
#include "display/qt/widgets/filter_widgets.h"
#include "display/qt/dialogs/filter_dialogs.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "filter/filter.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
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
static void filter_func_denoise_nlm(FILTER_FUNC_PARAMS);
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
// pixels, and a pointer to a user-facing GUI dialog with which the user can adjust
// the filter's parameters.
//
// Note: Each filter is identified by a UUID string. A UUID must be unique to a filter.
// The UUID of a filter must never be changed - a filter must remain identifiable with
// its initially-set UUID.
//
static const std::map<std::string, std::pair<filter_function_t, filter_dlg_s*>> FILTERS =
       {{"fc85a109-c57a-4317-994f-786652231773", {filter_func_deltahistogram, []{ static filter_dlg_deltahistogram_s f; return &f; }()}},
        {"badb0129-f48c-4253-a66f-b0ec94e225a0", {filter_func_uniquecount,    []{ static filter_dlg_uniquecount_s f; return &f;    }()}},
        {"03847778-bb9c-4e8c-96d5-0c10335c4f34", {filter_func_unsharpmask,    []{ static filter_dlg_unsharpmask_s f; return &f;    }()}},
        {"a5426f2e-b060-48a9-adf8-1646a2d3bd41", {filter_func_blur,           []{ static filter_dlg_blur_s f; return &f;           }()}},
        {"eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03", {filter_func_decimate,       []{ static filter_dlg_decimate_s f; return &f;       }()}},
        {"94adffac-be42-43ac-9839-9cc53a6d615c", {filter_func_denoise,        []{ static filter_dlg_denoise_s f; return &f;        }()}},
        {"e31d5ee3-f5df-4e7c-81b8-227fc39cbe76", {filter_func_denoise_nlm,    []{ static filter_dlg_denoise_nlm_s f; return &f;    }()}},
        {"1c25bbb1-dbf4-4a03-93a1-adf24b311070", {filter_func_sharpen,        []{ static filter_dlg_sharpen_s f; return &f;        }()}},
        {"de60017c-afe5-4e5e-99ca-aca5756da0e8", {filter_func_median,         []{ static filter_dlg_median_s f; return &f;         }()}},
        {"2448cf4a-112d-4d70-9fc1-b3e9176b6684", {filter_func_crop,           []{ static filter_dlg_crop_s f; return &f;           }()}},
        {"80a3ac29-fcec-4ae0-ad9e-bbd8667cc680", {filter_func_flip,           []{ static filter_dlg_flip_s f; return &f;           }()}},
        {"140c514d-a4b0-4882-abc6-b4e9e1ff4451", {filter_func_rotate,         []{ static filter_dlg_rotate_s f; return &f;         }()}}};

// All filter types available for use.
/// NOTE: This is intended for use by the new node graph-based filters dialog that
/// is currently being worked on but is not yet finalized.
static const std::unordered_map<std::string, const filter_meta_s> KNOWN_FILTER_TYPES =
{
    {"a5426f2e-b060-48a9-adf8-1646a2d3bd41", {"Blur",              filter_type_enum_e::blur,                   filter_func_blur           }},
    {"fc85a109-c57a-4317-994f-786652231773", {"Delta histogram",   filter_type_enum_e::delta_histogram,        filter_func_deltahistogram }},
    {"badb0129-f48c-4253-a66f-b0ec94e225a0", {"Unique count",      filter_type_enum_e::unique_count,           filter_func_uniquecount    }},
    {"03847778-bb9c-4e8c-96d5-0c10335c4f34", {"Unsharp mask",      filter_type_enum_e::unsharp_mask,           filter_func_unsharpmask    }},
    {"eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03", {"Decimate",          filter_type_enum_e::decimate,               filter_func_decimate       }},
    {"94adffac-be42-43ac-9839-9cc53a6d615c", {"Denoise, temporal", filter_type_enum_e::denoise_temporal,       filter_func_denoise        }},
    {"e31d5ee3-f5df-4e7c-81b8-227fc39cbe76", {"Denoise, NLM",      filter_type_enum_e::denoise_nonlocal_means, filter_func_denoise_nlm    }},
    {"1c25bbb1-dbf4-4a03-93a1-adf24b311070", {"Sharpen",           filter_type_enum_e::sharpen,                filter_func_sharpen        }},
    {"de60017c-afe5-4e5e-99ca-aca5756da0e8", {"Median",            filter_type_enum_e::median,                 filter_func_median         }},
    {"2448cf4a-112d-4d70-9fc1-b3e9176b6684", {"Crop",              filter_type_enum_e::crop,                   filter_func_crop           }},
    {"80a3ac29-fcec-4ae0-ad9e-bbd8667cc680", {"Flip",              filter_type_enum_e::flip,                   filter_func_flip           }},
    {"140c514d-a4b0-4882-abc6-b4e9e1ff4451", {"Rotate",            filter_type_enum_e::rotate,                 filter_func_rotate         }},

    {"136deb34-ac79-46b1-a09c-d57dcfaa84ad", {"Input gate",        filter_type_enum_e::input_gate,             nullptr                    }},
    {"be8443e2-4355-40fd-aded-63cebcbfb8ce", {"Output gate",       filter_type_enum_e::output_gate,            nullptr                    }},
};

static std::vector<filter_set_s*> FILTER_SETS;

// The index in the list of filter sets of the set which was most recently used.
// Generally, this will be the filter set that matches the current input/output
// resolution.
static int MOST_RECENT_FILTER_SET_IDX = -1;

// All filters expect 32-bit color, i.e. 4 channels.
const uint NUM_COLOR_CHANNELS = (32 / 8);

// All filters the user has added to the filter graph.
static std::vector<filter_c*> FILTER_POOL;

// Sets of filters, starting with an input gate, followed by a number of filters,
// and ending with an output gate. These chains will be used to filter incoming
// frames.
static std::vector<std::vector<const filter_c*>> FILTER_CHAINS;

std::string kf_filter_name_for_type(const filter_type_enum_e type)
{
    for (const auto filterType: KNOWN_FILTER_TYPES)
    {
        if (filterType.second.type == type)
        {
            return filterType.second.name;
        }
    }

    k_assert(0, "Unable to find a name for the given filter type.");

    return "(unknown)";
}

// Apply to the given pixel buffer the chain of filters (if any) whose input gate
// matches the frame's resolution and output gate that of the current output resolution.
void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r)
{
    if (!FILTERING_ENABLED) return;

    k_assert((r.bpp == 32), "Filters can only be applied to 32-bit pixel data.");

    for (const auto filterChain: FILTER_CHAINS)
    {
        const unsigned inputGateWidth = *(u16*)&(filterChain.front()->parameterData[0]);
        const unsigned inputGateHeight = *(u16*)&(filterChain.front()->parameterData[2]);

        if ((r.w != inputGateWidth) || (r.h != inputGateHeight))
        {
            continue;
        }

        const unsigned outputGateWidth = *(u16*)&(filterChain.back()->parameterData[0]);
        const unsigned outputGateHeight = *(u16*)&(filterChain.back()->parameterData[2]);
        const resolution_s outputRes = ks_output_resolution();

        if ((outputRes.w != outputGateWidth) || (outputRes.h != outputGateHeight))
        {
            continue;
        }

        // The gate filters are expected to be #first and #last, while the actual
        // applicable filters are the ones in-between.
        for (unsigned i = 1; i < (filterChain.size() - 1); i++)
        {
            filterChain[i]->metaData.apply(pixels, &r, filterChain[i]->parameterData.ptr());
        }

        return;
    }

    return;
}

std::vector<const filter_meta_s*> kf_known_filter_types(void)
{
    std::vector<const filter_meta_s*> filtersMetadata;

    for (const auto &filter: KNOWN_FILTER_TYPES)
    {
        filtersMetadata.push_back(&filter.second);
    }

    return filtersMetadata;
}

void kf_add_filter_chain(std::vector<const filter_c*> newChain)
{
    k_assert((newChain.size() >= 2) &&
             (newChain.at(0)->metaData.type == filter_type_enum_e::input_gate) &&
             (newChain.at(newChain.size()-1)->metaData.type == filter_type_enum_e::output_gate),
             "Detected a malformed filter chain.");

    FILTER_CHAINS.push_back(newChain);

    return;
}

void kf_remove_all_filter_chains(void)
{
    FILTER_CHAINS.clear();

    return;
}

const filter_c* kf_create_new_filter_instance(const char *const id)
{
    filter_c *filter = new filter_c(id);

    FILTER_POOL.push_back(filter);

    return filter;
}

const filter_c* kf_create_new_filter_instance(const filter_type_enum_e type)
{
    filter_c *newFilterInstance = nullptr;

    for (const auto filter: KNOWN_FILTER_TYPES)
    {
        if (filter.second.type == type)
        {
            newFilterInstance = new filter_c(filter.first);
            break;
        }
    }

    k_assert(newFilterInstance, "Failed to create a filter of the given type.");

    FILTER_POOL.push_back(newFilterInstance);

    return newFilterInstance;
}

void kf_delete_filter(const filter_c *const filter)
{
    const auto filterEntry = std::find(FILTER_POOL.begin(), FILTER_POOL.end(), filter);

    k_assert(filterEntry != FILTER_POOL.end(),"Was asked to delete an unknown filter.");

    FILTER_POOL.erase(filterEntry);

    kd_refresh_filter_chains();

    return;
}

// Returns a list of the UUIDs of all user-facing filters.
//
std::vector<std::string> kf_filter_uuid_list(void)
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

// Returns the display name of the filter identified by the given UUID.
//
std::string kf_filter_name_for_uuid(const std::string uuid)
{
    return FILTERS.at(uuid).second->name();
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

    for (auto filter: FILTER_POOL)
    {
        delete filter;
    }

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
    k_assert((idx < FILTER_SETS.size()), "Attempting to remove a filter set out of bounds.");

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
        const u32 idx = i * NUM_COLOR_CHANNELS;

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

// Non-local means denoising. Slow.
//
static void filter_func_denoise_nlm(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #if USE_OPENCV
        const u8 h = params[filter_dlg_denoise_nlm_s::OFFS_H];
        const u8 hColor = params[filter_dlg_denoise_nlm_s::OFFS_H_COLOR];
        const u8 templateWindowSize = params[filter_dlg_denoise_nlm_s::OFFS_TEMPLATE_WINDOW_SIZE];
        const u8 searchWindowSize = params[filter_dlg_denoise_nlm_s::OFFS_SEARCH_WINDOW_SIZE];

        cv::Mat input = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::fastNlMeansDenoisingColored(input, input, h, hColor, templateWindowSize, searchWindowSize);
    #endif

    return;
}

// Reduce temporal image noise by requiring that pixels between frames vary by at least
// a threshold value before being updated on screen.
//
static void filter_func_denoise(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 threshold = params[filter_dlg_denoise_s::OFFS_THRESHOLD];
    static heap_bytes_s<u8> prevPixels(MAX_FRAME_SIZE, "Denoising filter buffer");

    for (uint i = 0; i < (r->h * r->w); i++)
    {
        const u32 idx = i * NUM_COLOR_CHANNELS;

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
        const uint idx = i * NUM_COLOR_CHANNELS;
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
                        const u32 idx = ((x + xd) + (y + yd) * r->w) * NUM_COLOR_CHANNELS;

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
                const u32 idx = (x + y * r->w) * NUM_COLOR_CHANNELS;

                ab = pixels[idx + 0];
                ag = pixels[idx + 1];
                ar = pixels[idx + 2];
            }

            for (int yd = 0; yd < factor; yd++)
            {
                for (int xd = 0; xd < factor; xd++)
                {
                    const u32 idx = ((x + xd) + (y + yd) * r->w) * NUM_COLOR_CHANNELS;

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
            cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
            cv::Mat cropped = output(cv::Rect(x, y, w, h)).clone();

            // If the user doesn't want scaling, just append some black borders around the
            // cropping. Otherwise, stretch the cropped region to fill the entire frame.
            if (scaler < 0) cv::copyMakeBorder(cropped, output, y, (r->h - (h + y)), x, (r->w - (w + x)), cv::BORDER_CONSTANT, 0);
            else cv::resize(cropped, output, output.size(), 0, 0, scaler);
        }
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
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
    #else
        (void)axis;
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
    #else
        (void)angle;
        (void)scale;
    #endif

    return;
}

static void filter_func_median(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 kernelS = params[filter_dlg_median_s::OFFS_KERNEL_SIZE];

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::medianBlur(output, output, kernelS);
#endif

    return;
}

static void filter_func_blur(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const real kernelS = (params[filter_dlg_blur_s::OFFS_KERNEL_SIZE] / 10.0);

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);

    if (params[filter_dlg_blur_s::OFFS_TYPE] == filter_dlg_blur_s::FILTER_TYPE_GAUSSIAN)
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

bool kf_filter_exists(const std::string &name)
{
    // If we reach the end of the functions list before finding one by the given
    // name, the entry doesn't exist.
    const bool exists = (FILTERS.find(name) != FILTERS.end());

    return exists;
}

const filter_dlg_s* kf_filter_dialog_for_uuid(const std::string &uuid)
{
    return FILTERS.at(uuid).second;
}

// Returns a pointer to the filter function that corresponds to the given filter
// UUID.
//
filter_function_t kf_filter_function_ptr_for_uuid(const std::string &uuid)
{
    return FILTERS.at(uuid).first;
}

// Returns the filter UUID corresponding to the given filter name. If no such name
// could be found among the filters, an empty string will be returned.
//
std::string kf_filter_uuid_for_name(const std::string &name)
{
    for (const auto filter: FILTERS)
    {
        const auto uuid = filter.first;

        if (kf_filter_name_for_uuid(uuid) == name)
        {
            return uuid;
        }
    }

    return "";
}

void kf_set_filtering_enabled(const bool enabled)
{
    FILTERING_ENABLED = enabled;

    return;
}

static void apply_filter(const std::string &uuid, FILTER_FUNC_PARAMS)
{
    filter_function_t applyFn = kf_filter_function_ptr_for_uuid(uuid);

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

int kf_current_filter_set_idx(void)
{
    return MOST_RECENT_FILTER_SET_IDX;
}

// Returns a pointer to the filter set which the current input/output capture
// resolutions activate, or nullptr if no such match exists or if filtering is
// altogether disabled.
const filter_set_s* kf_current_filter_set(void)
{
    if (!FILTERING_ENABLED) return nullptr;

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
        apply_filter(f.uuid, pixels, &r, f.data);
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
        apply_filter(f.uuid, pixels, &r, f.data);
    }

    return;
}

void kf_initialize_filters(void)
{
    DEBUG(("Initializing custom filtering."));

    return;
}

filter_c::filter_c(const std::string &id) :
    metaData(KNOWN_FILTER_TYPES.at(id)),
    parameterData(heap_bytes_s<u8>(FILTER_DATA_LENGTH, "Filter parameter data")),
    guiWidget(new_gui_widget())
{
    this->guiWidget->reset_parameter_data();

    return;
}

filter_c::~filter_c()
{
    delete this->guiWidget;
    this->parameterData.release_memory();

    return;
}

filter_widget_s *filter_c::new_gui_widget(void)
{
    u8 *const paramData = this->parameterData.ptr();

    switch (this->metaData.type)
    {
        case filter_type_enum_e::blur:                   return new filter_widget_blur_s(paramData);
        case filter_type_enum_e::rotate:                 return new filter_widget_rotate_s(paramData);
        case filter_type_enum_e::crop:                   return new filter_widget_crop_s(paramData);
        case filter_type_enum_e::flip:                   return new filter_widget_flip_s(paramData);
        case filter_type_enum_e::median:                 return new filter_widget_median_s(paramData);
        case filter_type_enum_e::denoise_temporal:       return new filter_widget_denoise_temporal_s(paramData);
        case filter_type_enum_e::denoise_nonlocal_means: return new filter_widget_denoise_nonlocal_means_s(paramData);
        case filter_type_enum_e::sharpen:                return new filter_widget_sharpen_s(paramData);
        case filter_type_enum_e::unsharp_mask:           return new filter_widget_unsharp_mask_s(paramData);
        case filter_type_enum_e::decimate:               return new filter_widget_decimate_s(paramData);
        case filter_type_enum_e::delta_histogram:        return new filter_widget_delta_histogram_s(paramData);
        case filter_type_enum_e::unique_count:           return new filter_widget_unique_count_s(paramData);

        case filter_type_enum_e::input_gate:             return new filter_widget_input_gate_s(paramData);
        case filter_type_enum_e::output_gate:            return new filter_widget_output_gate_s(paramData);

        default: k_assert(0, "No GUI widget is yet available for the given filter type.");
    }
}
