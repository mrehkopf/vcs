/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_unsharp_mask_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    static heap_mem<u8> TMP_BUF(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Unsharp mask buffer");

    const double str = this->parameter(PARAM_STRENGTH);
    const double rad = this->parameter(PARAM_RADIUS);
    cv::Mat tmp = cv::Mat(r.h, r.w, CV_8UC4, TMP_BUF.data());
    cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
    
    cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
    cv::addWeighted(output, 1 + str, tmp, -str, 0, output);

    return;
}
