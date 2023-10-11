/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"
#include <opencv2/photo/photo.hpp>

// Quite slow.
void filter_denoise_nonlocal_means_c::apply(image_s *const image)
{
    const double h = this->parameter(PARAM_H);
    const double hColor = this->parameter(PARAM_H_COLOR);
    const double templateWindowSize = this->parameter(PARAM_TEMPLATE_WINDOW_SIZE);
    const double searchWindowSize = this->parameter(PARAM_SEARCH_WINDOW_SIZE);

    cv::Mat input = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::fastNlMeansDenoisingColored(input, input, h, hColor, templateWindowSize, searchWindowSize);

    return;
}
