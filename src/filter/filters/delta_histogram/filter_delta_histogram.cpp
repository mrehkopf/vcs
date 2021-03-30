/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/delta_histogram/filter_delta_histogram.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// Draws a histogram by color value of the number of pixels changed between frames.
void filter_delta_histogram_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        static heap_bytes_s<u8> prevFramePixels(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Delta histogram buffer");

        const unsigned numBins = 512;
        const unsigned numColorChannels = (r.bpp / 8);

        // For each RGB channel, count into bins how many times a particular delta
        // between pixels in the previous frame and this one occurred.
        uint bl[numBins] = {0};
        uint gr[numBins] = {0};
        uint re[numBins] = {0};
        for (uint i = 0; i < (r.w * r.h); i++)
        {
            const uint idx = i * numColorChannels;
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
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        for (uint i = 1; i < numBins; i++)
        {
            const uint maxval = r.w * r.h;
            real xskip = (r.w / (real)numBins);

            uint x2 = xskip * i;
            uint x1 = x2-xskip;

            const uint y1b = r.h - ((r.h / 256.0) * ((256.0 / maxval) * bl[i-1]));
            const uint y2b = r.h - ((r.h / 256.0) * ((256.0 / maxval) * bl[i]));

            const uint y1g = r.h - ((r.h / 256.0) * ((256.0 / maxval) * gr[i-1]));
            const uint y2g = r.h - ((r.h / 256.0) * ((256.0 / maxval) * gr[i]));

            const uint y1r = r.h - ((r.h / 256.0) * ((256.0 / maxval) * re[i-1]));
            const uint y2r = r.h - ((r.h / 256.0) * ((256.0 / maxval) * re[i]));

            cv::line(output, cv::Point(x1, y1b), cv::Point(x2, y2b), cv::Scalar(255, 0, 0), 2, CV_AA);
            cv::line(output, cv::Point(x1, y1g), cv::Point(x2, y2g), cv::Scalar(0, 255, 0), 2, CV_AA);
            cv::line(output, cv::Point(x1, y1r), cv::Point(x2, y2r), cv::Scalar(0, 0, 255), 2, CV_AA);
        }

        memcpy(prevFramePixels.ptr(), pixels, prevFramePixels.up_to(r.w * r.h * numColorChannels));
    #endif

    return;
}
