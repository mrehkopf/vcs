/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/crop/filter_crop.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// Takes a subregion of the frame and either scales it up to fill the whole frame or
// fills its surroundings with black.
void filter_crop_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    const unsigned x = this->parameter(PARAM_X);
    const unsigned y = this->parameter(PARAM_Y);
    const unsigned w = this->parameter(PARAM_WIDTH);
    const unsigned h = this->parameter(PARAM_HEIGHT);
    const unsigned scalerType = this->parameter(PARAM_SCALER);

    #ifdef USE_OPENCV
        int cvScaler = -1;

        switch (scalerType)
        {
            case 0: cvScaler = cv::INTER_LINEAR; break;
            case 1: cvScaler = cv::INTER_NEAREST; break;
            case 2: cvScaler = -1 /*Don't scale.*/; break;
            default: k_assert(0, "Unknown scaler type for the crop filter."); break;
        }

        if (((x + w) > r.w) || ((y + h) > r.h))
        {
            /// TODO: Signal a user-facing but non-obtrusive message about the crop
            /// params being invalid.
        }
        else
        {
            cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
            cv::Mat cropped = output(cv::Rect(x, y, w, h)).clone();

            // If the user doesn't want scaling, just append some black borders around the
            // cropping. Otherwise, stretch the cropped region to fill the entire frame.
            if (cvScaler < 0) cv::copyMakeBorder(cropped, output, y, (r.h - (h + y)), x, (r.w - (w + x)), cv::BORDER_CONSTANT, 0);
            else cv::resize(cropped, output, output.size(), 0, 0, cvScaler);
        }
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    #endif

    return;
}
