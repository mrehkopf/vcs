/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/color_depth/filter_color_depth.h"

void filter_color_depth_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const unsigned maskRed   = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_RED)));
    const unsigned maskGreen = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_GREEN)));
    const unsigned maskBlue  = (255u << unsigned(8 - this->parameter(PARAM_BIT_COUNT_BLUE)));
    const unsigned numImageBytes = (image->resolution.w * image->resolution.h * (image->bitsPerPixel / 8));

    // Assumes 32-bit pixels (BGRA8888).
    for (unsigned i = 0; i < numImageBytes; i += 4)
    {
        image->pixels[i + 0] &= maskBlue;
        image->pixels[i + 1] &= maskGreen;
        image->pixels[i + 2] &= maskRed;
    }

    return;
}
