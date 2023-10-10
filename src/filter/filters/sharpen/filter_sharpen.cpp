/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/sharpen/filter_sharpen.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_sharpen_c::apply(image_s *const image)
{
    ASSERT_FILTER_ARGUMENTS(image);

    float kernel[] = {
        0, -1,  0,
       -1,  5, -1,
        0, -1,  0
    };

    cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::filter2D(output, output, -1, ker);

    return;
}
