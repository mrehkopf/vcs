/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/flip/filter_flip.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// Flips the frame horizontally and/or vertically.
void filter_flip_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static heap_bytes_s<u8> scratch(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Flip filter scratch buffer");

    int axis = this->parameter(PARAM_AXIS);

    // 0 = vertical, 1 = horizontal, -1 = both.
    axis = ((axis == 2)? -1 : axis);

    #ifdef USE_OPENCV
        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::Mat temp = cv::Mat(r.h, r.w, CV_8UC4, scratch.data());

        cv::flip(output, temp, axis);
        temp.copyTo(output);
    #else
        (void)axis;
    #endif

    return;
}
