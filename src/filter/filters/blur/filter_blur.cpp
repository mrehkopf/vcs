/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/blur/filter_blur.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_blur_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);

        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);

        if (this->parameter(PARAM_TYPE) == BLUR_GAUSSIAN)
        {
            cv::GaussianBlur(output, output, cv::Size(0, 0), kernelSize);
        }
        else
        {
            const u8 kernelW = ((int(kernelSize) * 2) + 1);
            cv::blur(output, output, cv::Size(kernelW, kernelW));
        }
    #endif

    return;
}
