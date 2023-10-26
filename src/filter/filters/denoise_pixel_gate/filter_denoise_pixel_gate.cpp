/*
 * 2021, 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <opencv2/core.hpp>
#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "common/globals.h"

// Reduces temporal image noise by requiring that pixels between frames vary by at
// least a threshold value before being updated on screen.
void filter_denoise_pixel_gate_c::apply(image_s *const image)
{
    const unsigned threshold = this->parameter(PARAM_THRESHOLD);
    static uint8_t *const prevPixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
    static uint8_t *const absoluteDiff = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    cv::absdiff(
        cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels),
        cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, prevPixels),
        cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, absoluteDiff)
    );

    for (unsigned i = 0; i < image->byte_size(); i += 4)
    {
        if (
            (absoluteDiff[i + 0] > threshold) ||
            (absoluteDiff[i + 1] > threshold) ||
            (absoluteDiff[i + 2] > threshold)
        ){
            prevPixels[i + 0] = image->pixels[i + 0];
            prevPixels[i + 1] = image->pixels[i + 1];
            prevPixels[i + 2] = image->pixels[i + 2];
        }
        else
        {
            image->pixels[i + 0] = prevPixels[i + 0];
            image->pixels[i + 1] = prevPixels[i + 1];
            image->pixels[i + 2] = prevPixels[i + 2];
        }
    }

    return;
}
