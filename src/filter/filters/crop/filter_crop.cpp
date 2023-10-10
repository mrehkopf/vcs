/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/crop/filter_crop.h"
#include <opencv2/imgproc/imgproc.hpp>
#include "scaler/scaler.h"

// Takes a subregion of the frame and either scales it up to fill the whole frame or
// fills its surroundings with black.
void filter_crop_c::apply(image_s *const image)
{
    ASSERT_FILTER_ARGUMENTS(image);

    const unsigned x = this->parameter(PARAM_X);
    const unsigned y = this->parameter(PARAM_Y);
    const unsigned w = this->parameter(PARAM_WIDTH);
    const unsigned h = this->parameter(PARAM_HEIGHT);
    const unsigned scalerType = this->parameter(PARAM_SCALER);

    const int cvScaler = ([scalerType]
    {
        switch (scalerType)
        {
            case SCALE_LINEAR: return cv::INTER_LINEAR;
            case SCALE_NEAREST: return cv::INTER_NEAREST;
            case SCALE_NONE: return cv::INTER_NEAREST;
            default: k_assert(0, "Unknown scaler type for the crop filter."); return cv::INTER_NEAREST;
        }
    })();

    if ((x + w) > image->resolution.w)
    {
        KS_PRINT_FILTER_ERROR("The crop region is wider than the input image");
    }
    else if ((y + h) > image->resolution.h)
    {
        KS_PRINT_FILTER_ERROR("The crop region is taller than the input image");
    }
    else
    {
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::Mat cropped = output(cv::Rect(x, y, w, h)).clone();

        // If the user doesn't want scaling, just append some black borders around the
        // cropping. Otherwise, stretch the cropped region to fill the entire frame.
        if (scalerType == SCALE_NONE) cv::copyMakeBorder(cropped, output, y, (image->resolution.h - (h + y)), x, (image->resolution.w - (w + x)), cv::BORDER_CONSTANT, 0);
        else cv::resize(cropped, output, output.size(), 0, 0, cvScaler);
    }

    return;
}
