/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/color_depth/filter_color_depth.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_color_depth_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    const unsigned maskRed   = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_RED)));
    const unsigned maskGreen = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_GREEN)));
    const unsigned maskBlue  = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_BLUE)));

    const unsigned numImageBytes = (r.w * r.h * (r.bpp / 8));

    // Assumes 32-bit pixels (BGRA8888).
    for (unsigned i = 0; i < numImageBytes; i += 4)
    {
        pixels[i + 0] &= maskBlue;
        pixels[i + 1] &= maskGreen;
        pixels[i + 2] &= maskRed;
    }

    return;
}
