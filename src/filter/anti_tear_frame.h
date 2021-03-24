/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_FRAME_H
#define VCS_FILTER_ANTI_TEAR_FRAME_H

#include "common/globals.h"
#include "common/memory/memory_interface.h"

struct anti_tear_frame_s
{
    resolution_s resolution;

    heap_bytes_s<u8> pixels;

    anti_tear_frame_s(const resolution_s resolution = {0, 0, 0},
                      u8 *const pixels = nullptr)
    {
        this->resolution = resolution;

        if (pixels)
        {
            this->pixels.point_to(pixels, (resolution.w * resolution.h * (resolution.bpp / 8)));
        }

        return;
    }

    void flip_vertically(void)
    {
        u8 scratch[MAX_OUTPUT_WIDTH];

        for (unsigned y = 0; y < (this->resolution.h / 2); y++)
        {
            const unsigned numBytesPerPixel = (this->resolution.bpp / 8);
            const unsigned numBytesPerRow = (this->resolution.w * numBytesPerPixel);
            const unsigned idx1 = (y * this->resolution.w * numBytesPerPixel);
            const unsigned idx2 = ((this->resolution.h - 1 - y) * this->resolution.w * numBytesPerPixel);

            std::memcpy(&scratch,                    (this->pixels.ptr() + idx1), numBytesPerRow);
            std::memcpy((this->pixels.ptr() + idx1), (this->pixels.ptr() + idx2), numBytesPerRow);
            std::memcpy((this->pixels.ptr() + idx2), &scratch, numBytesPerRow);
        }

        return;
    }
};

#endif
