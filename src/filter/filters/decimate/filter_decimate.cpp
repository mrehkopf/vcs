/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/decimate/filter_decimate.h"

void filter_decimate_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const int factor = this->parameter(PARAM_FACTOR);
    const unsigned type = this->parameter(PARAM_TYPE);
    const unsigned numColorChannels = (image->bitsPerPixel / 8);

    for (unsigned y = 0; y < image->resolution.h; y += factor)
    {
        for (unsigned x = 0; x < image->resolution.w; x += factor)
        {
            int ar = 0, ag = 0, ab = 0;

            if (type == SAMPLE_AVERAGE)
            {
                for (int yd = 0; yd < factor; yd++)
                {
                    for (int xd = 0; xd < factor; xd++)
                    {
                        const unsigned idx = ((x + xd) + (y + yd) * image->resolution.w) * numColorChannels;
                        ab += image->pixels[idx + 0];
                        ag += image->pixels[idx + 1];
                        ar += image->pixels[idx + 2];
                    }
                }

                ar /= (factor * factor);
                ag /= (factor * factor);
                ab /= (factor * factor);
            }
            else if (type == SAMPLE_NEAREST)
            {
                const unsigned idx = (x + y * image->resolution.w) * numColorChannels;
                ab = image->pixels[idx + 0];
                ag = image->pixels[idx + 1];
                ar = image->pixels[idx + 2];
            }

            for (int yd = 0; yd < factor; yd++)
            {
                for (int xd = 0; xd < factor; xd++)
                {
                    const unsigned idx = ((x + xd) + (y + yd) * image->resolution.w) * numColorChannels;
                    image->pixels[idx + 0] = ab;
                    image->pixels[idx + 1] = ag;
                    image->pixels[idx + 2] = ar;
                }
            }
        }
    }

    return;
}
