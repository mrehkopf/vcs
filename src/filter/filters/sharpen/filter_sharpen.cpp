/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/sharpen/filter_sharpen.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_sharpen_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        float kernel[] = { 0, -1,  0,
                          -1,  5, -1,
                           0, -1,  0};

        cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::filter2D(output, output, -1, ker);
    #endif

    return;
}
