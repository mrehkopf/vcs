/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// Quite slow.
void filter_denoise_nonlocal_means_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    #if USE_OPENCV
        const u8 h = this->parameter(PARAM_H);
        const u8 hColor = this->parameter(PARAM_H_COLOR);
        const u8 templateWindowSize = this->parameter(PARAM_TEMPLATE_WINDOW_SIZE);
        const u8 searchWindowSize = this->parameter(PARAM_SEARCH_WINDOW_SIZE);

        cv::Mat input = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::fastNlMeansDenoisingColored(input, input, h, hColor, templateWindowSize, searchWindowSize);
    #endif

    return;
}
