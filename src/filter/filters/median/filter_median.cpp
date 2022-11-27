/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/median/filter_median.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_median_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::medianBlur(output, output, kernelSize);

    return;
}
