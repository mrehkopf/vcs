/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_unsharp_mask_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        static heap_bytes_s<u8> TMP_BUF(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Unsharp mask buffer");

        const double str = (this->parameter(PARAM_STRENGTH) / 100.0);
        const double rad = (this->parameter(PARAM_RADIUS) / 10.0);

        cv::Mat tmp = cv::Mat(r.h, r.w, CV_8UC4, TMP_BUF.ptr());
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
        cv::addWeighted(output, 1 + str, tmp, -str, 0, output);
    #endif

    return;
}
