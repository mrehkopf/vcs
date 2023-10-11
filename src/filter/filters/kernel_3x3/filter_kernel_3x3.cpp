/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/kernel_3x3/filter_kernel_3x3.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_kernel_3x3_c::apply(image_s *const image)
{
    const float v11 = this->parameter(PARAM_11);
    const float v12 = this->parameter(PARAM_12);
    const float v13 = this->parameter(PARAM_13);
    const float v21 = this->parameter(PARAM_21);
    const float v22 = this->parameter(PARAM_22);
    const float v23 = this->parameter(PARAM_23);
    const float v31 = this->parameter(PARAM_31);
    const float v32 = this->parameter(PARAM_32);
    const float v33 = this->parameter(PARAM_33);

    float kernel[] = {
        v11, v12, v13,
        v21, v22, v23,
        v31, v32, v33
    };

    cv::Mat ker = cv::Mat(3, 3, CV_32F, &kernel);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::filter2D(output, output, -1, ker);

    return;
}
