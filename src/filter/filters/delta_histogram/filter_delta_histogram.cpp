/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/delta_histogram/filter_delta_histogram.h"
#include <opencv2/imgproc/imgproc.hpp>

// Draws a histogram by color value of the number of pixels changed between frames.
void filter_delta_histogram_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static heap_mem<u8> prevFramePixels(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Delta histogram buffer");

    const unsigned numBins = 512;
    const unsigned numColorChannels = (image->resolution.bpp / 8);

    // For each RGB channel, count into bins how many times a particular delta
    // between pixels in the previous frame and this one occurred.
    uint bl[numBins] = {0};
    uint gr[numBins] = {0};
    uint re[numBins] = {0};
    for (uint i = 0; i < (image->resolution.w * image->resolution.h); i++)
    {
        const uint idx = i * numColorChannels;
        const uint deltaBlue = (image->pixels[idx + 0] - prevFramePixels[idx + 0]) + 255;
        const uint deltaGreen = (image->pixels[idx + 1] - prevFramePixels[idx + 1]) + 255;
        const uint deltaRed = (image->pixels[idx + 2] - prevFramePixels[idx + 2]) + 255;

        k_assert(deltaBlue < numBins, "");
        k_assert(deltaGreen < numBins, "");
        k_assert(deltaRed < numBins, "");

        bl[deltaBlue]++;
        gr[deltaGreen]++;
        re[deltaRed]++;
    }

    // Draw the bins into the frame as a line graph.
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    for (uint i = 1; i < numBins; i++)
    {
        const uint maxval = image->resolution.w * image->resolution.h;
        real xskip = (image->resolution.w / (real)numBins);

        uint x2 = xskip * i;
        uint x1 = x2-xskip;

        const uint y1b = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * bl[i-1]));
        const uint y2b = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * bl[i]));

        const uint y1g = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * gr[i-1]));
        const uint y2g = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * gr[i]));

        const uint y1r = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * re[i-1]));
        const uint y2r = image->resolution.h - ((image->resolution.h / 256.0) * ((256.0 / maxval) * re[i]));

        cv::line(output, cv::Point(x1, y1b), cv::Point(x2, y2b), cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
        cv::line(output, cv::Point(x1, y1g), cv::Point(x2, y2g), cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
        cv::line(output, cv::Point(x1, y1r), cv::Point(x2, y2r), cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
    }

    memcpy(prevFramePixels.data(), image->pixels, prevFramePixels.size_check(image->resolution.w * image->resolution.h * numColorChannels));

    return;
}
