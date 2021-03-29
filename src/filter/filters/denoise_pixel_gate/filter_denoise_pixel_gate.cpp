/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"

// Reduces temporal image noise by requiring that pixels between frames vary by at
// least a threshold value before being updated on screen.
void filter_denoise_pixel_gate_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    const unsigned threshold = this->parameter(PARAM_THRESHOLD);
    static heap_bytes_s<u8> prevPixels(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Denoising filter buffer");

    for (uint i = 0; i < (r.h * r.w); i++)
    {
        const u32 idx = (i * (r.bpp / 8));

        if ((abs(pixels[idx + 0] - prevPixels[idx + 0]) > threshold) ||
            (abs(pixels[idx + 1] - prevPixels[idx + 1]) > threshold) ||
            (abs(pixels[idx + 2] - prevPixels[idx + 2]) > threshold))
        {
            prevPixels[idx + 0] = pixels[idx + 0];
            prevPixels[idx + 1] = pixels[idx + 1];
            prevPixels[idx + 2] = pixels[idx + 2];
        }
        else
        {
            pixels[idx + 0] = prevPixels[idx + 0];
            pixels[idx + 1] = prevPixels[idx + 1];
            pixels[idx + 2] = prevPixels[idx + 2];
        }
    }

    return;
}
