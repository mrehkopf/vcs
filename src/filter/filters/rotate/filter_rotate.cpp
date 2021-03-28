/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/rotate/filter_rotate.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_rotate_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Rotate filter scratch buffer");

    const double angle = (this->parameter(PARAM_ROT) / 10.0);
    const double scale = (this->parameter(PARAM_SCALE) / 100.0);

    #ifdef USE_OPENCV
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::Mat temp = cv::Mat(r.h, r.w, CV_8UC4, scratch.ptr());

        cv::Mat transf = cv::getRotationMatrix2D(cv::Point2d((r.w / 2), (r.h / 2)), -angle, scale);
        cv::warpAffine(output, temp, transf, cv::Size(r.w, r.h));
        temp.copyTo(output);
    #else
        (void)angle;
        (void)scale;
    #endif

    return;
}
