/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/decimate/filter_decimate.h"

void filter_decimate_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        const unsigned factor = this->parameter(PARAM_FACTOR);
        const unsigned type = this->parameter(PARAM_TYPE);
        const unsigned numColorChannels = (r.bpp / 8);

        for (u32 y = 0; y < r.h; y += factor)
        {
            for (u32 x = 0; x < r.w; x += factor)
            {
                int ar = 0, ag = 0, ab = 0;

                if (type == SAMPLE_AVERAGE)
                {
                    for (int yd = 0; yd < factor; yd++)
                    {
                        for (int xd = 0; xd < factor; xd++)
                        {
                            const u32 idx = ((x + xd) + (y + yd) * r.w) * numColorChannels;

                            ab += pixels[idx + 0];
                            ag += pixels[idx + 1];
                            ar += pixels[idx + 2];
                        }
                    }
                    ar /= (factor * factor);
                    ag /= (factor * factor);
                    ab /= (factor * factor);
                }
                else if (type == SAMPLE_NEAREST)
                {
                    const u32 idx = (x + y * r.w) * numColorChannels;

                    ab = pixels[idx + 0];
                    ag = pixels[idx + 1];
                    ar = pixels[idx + 2];
                }

                for (int yd = 0; yd < factor; yd++)
                {
                    for (int xd = 0; xd < factor; xd++)
                    {
                        const u32 idx = ((x + xd) + (y + yd) * r.w) * numColorChannels;

                        pixels[idx + 0] = ab;
                        pixels[idx + 1] = ag;
                        pixels[idx + 2] = ar;
                    }
                }
            }
        }
    #endif

    return;
}
