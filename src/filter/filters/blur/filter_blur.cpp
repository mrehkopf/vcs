/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/blur/filter_blur.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_blur_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);

    if (this->parameter(PARAM_TYPE) == BLUR_GAUSSIAN)
    {
        cv::GaussianBlur(output, output, cv::Size(0, 0), kernelSize);
    }
    else
    {
        const u8 kernelW = ((int(kernelSize) * 2) + 1);
        cv::blur(output, output, cv::Size(kernelW, kernelW));
    }

    return;
}
