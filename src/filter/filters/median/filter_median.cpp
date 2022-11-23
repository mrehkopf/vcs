/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/median/filter_median.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_median_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);
    cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
    cv::medianBlur(output, output, kernelSize);

    return;
}
