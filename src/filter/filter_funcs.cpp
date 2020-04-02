/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <ctime>
#include "common/globals.h"
#include "display/qt/widgets/filter_widgets.h"
#include "filter/filter_funcs.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// All filters expect 32-bit color, i.e. 4 channels.
static const unsigned NUM_COLOR_CHANNELS = (32 / 8);

// Invoke this macro at the start of each filter_func_*() function, to verify
// that the parameters passed are valid to operate on.
#define VALIDATE_FILTER_INPUT  k_assert(r->bpp == 32, "This filter expects 32-bit source color.");\
                               if (pixels == nullptr || params == nullptr || r == nullptr) return;


// Counts the number of unique frames per second, i.e. frames in which the pixels
// change between frames by less than a set threshold (which is to account for
// analog capture artefacts).
//
void filter_func_unique_count(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    static heap_bytes_s<u8> prevPixels(MAX_FRAME_SIZE, "Unique count filter buffer");

    const u8 threshold = params[filter_widget_unique_count_s::OFFS_THRESHOLD];
    const u8 corner = params[filter_widget_unique_count_s::OFFS_CORNER];

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
void filter_func_denoise_nonlocal_means(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #if USE_OPENCV
        const u8 h = params[filter_widget_denoise_nonlocal_means_s::OFFS_H];
        const u8 hColor = params[filter_widget_denoise_nonlocal_means_s::OFFS_H_COLOR];
        const u8 templateWindowSize = params[filter_widget_denoise_nonlocal_means_s::OFFS_TEMPLATE_WINDOW_SIZE];
        const u8 searchWindowSize = params[filter_widget_denoise_nonlocal_means_s::OFFS_SEARCH_WINDOW_SIZE];

        cv::Mat input = cv::Mat(r->h, r->w, CV_8UC4, pixels);
        cv::fastNlMeansDenoisingColored(input, input, h, hColor, templateWindowSize, searchWindowSize);
    #endif

    return;
}

// Reduce temporal image noise by requiring that pixels between frames vary by at least
// a threshold value before being updated on screen.
//
void filter_func_denoise_temporal(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 threshold = params[filter_widget_denoise_temporal_s::OFFS_THRESHOLD];
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
void filter_func_delta_histogram(FILTER_FUNC_PARAMS)
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

void filter_func_unsharp_mask(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    static heap_bytes_s<u8> TMP_BUF(MAX_FRAME_SIZE, "Unsharp mask buffer");
    const real str = params[filter_widget_unsharp_mask_s::OFFS_STRENGTH] / 100.0;
    const real rad = params[filter_widget_unsharp_mask_s::OFFS_RADIUS] / 10.0;

    cv::Mat tmp = cv::Mat(r->h, r->w, CV_8UC4, TMP_BUF.ptr());
    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
    cv::addWeighted(output, 1 + str, tmp, -str, 0, output);
#endif

    return;
}

void filter_func_sharpen(FILTER_FUNC_PARAMS)
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
void filter_func_decimate(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 factor = params[filter_widget_decimate_s::OFFS_FACTOR];
    const u8 type = params[filter_widget_decimate_s::OFFS_TYPE];

    for (u32 y = 0; y < r->h; y += factor)
    {
        for (u32 x = 0; x < r->w; x += factor)
        {
            int ar = 0, ag = 0, ab = 0;

            if (type == filter_widget_decimate_s::FILTER_TYPE_AVERAGE)
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
            else if (type == filter_widget_decimate_s::FILTER_TYPE_NEAREST)
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
void filter_func_crop(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    uint x = *(u16*)&(params[filter_widget_crop_s::OFFS_X]);
    uint y = *(u16*)&(params[filter_widget_crop_s::OFFS_Y]);
    uint w = *(u16*)&(params[filter_widget_crop_s::OFFS_WIDTH]);
    uint h = *(u16*)&(params[filter_widget_crop_s::OFFS_HEIGHT]);

    #ifdef USE_OPENCV
        int scaler = -1;
        switch (params[filter_widget_crop_s::OFFS_SCALER])
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
void filter_func_flip(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_FRAME_SIZE, "Flip filter scratch buffer");

    // 0 = vertical, 1 = horizontal, -1 = both.
    const uint axis = ((params[filter_widget_flip_s::OFFS_AXIS] == 2)? -1 : params[filter_widget_flip_s::OFFS_AXIS]);

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

void filter_func_rotate(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_FRAME_SIZE, "Rotate filter scratch buffer");

    const double angle = (*(i16*)&(params[filter_widget_rotate_s::OFFS_ROT]) / 10.0);
    const double scale = (*(i16*)&(params[filter_widget_rotate_s::OFFS_SCALE]) / 100.0);

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

void filter_func_median(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const u8 kernelS = params[filter_widget_median_s::OFFS_KERNEL_SIZE];

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);
    cv::medianBlur(output, output, kernelS);
#endif

    return;
}

void filter_func_blur(FILTER_FUNC_PARAMS)
{
    VALIDATE_FILTER_INPUT

#ifdef USE_OPENCV
    const real kernelS = (params[filter_widget_blur_s::OFFS_KERNEL_SIZE] / 10.0);

    cv::Mat output = cv::Mat(r->h, r->w, CV_8UC4, pixels);

    if (params[filter_widget_blur_s::OFFS_TYPE] == filter_widget_blur_s::FILTER_TYPE_GAUSSIAN)
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
