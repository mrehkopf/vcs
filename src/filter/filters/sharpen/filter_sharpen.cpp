/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/sharpen/filter_sharpen.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_sharpen_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    float kernel[] = {
        0, -1,  0,
       -1,  5, -1,
        0, -1,  0
    };

    cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
    cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
    cv::filter2D(output, output, -1, ker);

    return;
}
