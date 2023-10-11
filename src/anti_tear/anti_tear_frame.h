/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_FRAME_H
#define VCS_FILTER_ANTI_TEAR_FRAME_H

#include "common/globals.h"
#include "common/assert.h"
#include "display/display.h"

struct anti_tear_frame_s
{
    resolution_s resolution;
    uint8_t *pixels;
    const unsigned bitsPerPixel = 32;

    anti_tear_frame_s(const resolution_s resolution = {.w = 0, .h = 0}, uint8_t *const pixels = nullptr)
    {
        this->resolution = resolution;
        this->pixels = pixels;
    }

    bool is_valid(void)
    {
        return (
            this->pixels &&
            (this->resolution.w <= MAX_OUTPUT_WIDTH) &&
            (this->resolution.h <= MAX_OUTPUT_HEIGHT)
        );
    }

    void flip_vertically(void)
    {
        k_assert(this->is_valid(), "Invalid frame.");

        uint8_t scratch[MAX_OUTPUT_WIDTH * (MAX_OUTPUT_BPP / 8)];

        for (unsigned y = 0; y < (this->resolution.h / 2); y++)
        {
            const unsigned numBytesPerPixel = (this->bitsPerPixel / 8);
            const unsigned numBytesPerRow = (this->resolution.w * numBytesPerPixel);
            const unsigned idx1 = (y * this->resolution.w * numBytesPerPixel);
            const unsigned idx2 = ((this->resolution.h - 1 - y) * this->resolution.w * numBytesPerPixel);

            std::memcpy(&scratch,                    (this->pixels + idx1), numBytesPerRow);
            std::memcpy((this->pixels + idx1), (this->pixels + idx2), numBytesPerRow);
            std::memcpy((this->pixels + idx2), &scratch, numBytesPerRow);
        }

        return;
    }
};

#endif
