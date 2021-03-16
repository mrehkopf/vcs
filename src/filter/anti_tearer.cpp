/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <cstring>
#include "filter/anti_tearer.h"

anti_tearer_c::~anti_tearer_c(void)
{
    for (auto &buffer: this->buffers)
    {
        buffer.release_memory();
    }

    return;
}

void anti_tearer_c::initialize(const resolution_s &maxResolution)
{
    const unsigned requiredBufferSize = maxResolution.w * maxResolution.h * (maxResolution.bpp / 8);

    for (auto &buffer: this->buffers)
    {
        buffer.alloc(requiredBufferSize);
    }

    k_assert((this->buffers.size() >= 3), "Too few buffers.");
    this->backBuffer = this->buffers[0].ptr();
    this->frontBuffer = this->buffers[1].ptr();
    this->presentBuffer.pixels.point_to(this->buffers[2].ptr(), requiredBufferSize);

    this->presentBuffer.r = {0, 0, 32};

    this->onePerFrame.initialize(this);

    return;
}

u8* anti_tearer_c::pixels(void) const
{
    return this->presentBuffer.pixels.ptr();
}

void anti_tearer_c::copy_frame_pixel_rows(u8 *const dstBuffer,
                                          const captured_frame_s *const srcFrame,
                                          const unsigned fromRow,
                                          const unsigned toRow)
{
    if (fromRow == toRow)
    {
        return;
    }

    k_assert(((fromRow < toRow) && (toRow <= srcFrame->r.h)), "Malformed parameters.");

    const unsigned bpp = (srcFrame->r.bpp / 8);
    const unsigned idx = ((fromRow * srcFrame->r.w) * bpp);
    const unsigned numBytes = (((toRow - fromRow) * srcFrame->r.w) * bpp);
    std::memcpy((dstBuffer + idx), (srcFrame->pixels.ptr() + idx), numBytes);

    return;
}

int anti_tearer_c::find_tear_row(const captured_frame_s *const frame,
                                 unsigned startY,
                                 unsigned endY)
{
    for (unsigned rowIdx = startY; rowIdx < endY; rowIdx++)
    {
        if (this->is_pixel_row_new(rowIdx, frame))
        {
            // If the new row of pixels is at the top of the frame, there's no
            // tearing (we assume the frame fills in from bottom to top).
            return ((rowIdx == startY)? -1 : rowIdx);
        }
    }

    return -1;
}

bool anti_tearer_c::is_pixel_row_new(const unsigned rowIdx,
                                     const captured_frame_s *const frame)
{
    unsigned x = 0;
    unsigned matches = 0;
    const unsigned threshold = (this->windowLength * this->threshold);

    // Slide a sampling window across this horizontal row of pixels.
    while ((x + this->windowLength) < frame->r.w)
    {
        int oldR = 0, oldG = 0, oldB = 0;
        int newR = 0, newG = 0, newB = 0;

        // Find the average color values of the current and the previous frame
        // within this sampling window.
        for (size_t w = 0; w < this->windowLength; w++)
        {
            const unsigned bpp = (frame->r.bpp / 8);
            const unsigned idx = (((x + w) + rowIdx * frame->r.w) * bpp);

            oldB += this->frontBuffer[idx + 0];
            oldG += this->frontBuffer[idx + 1];
            oldR += this->frontBuffer[idx + 2];

            newB += frame->pixels[idx + 0];
            newG += frame->pixels[idx + 1];
            newR += frame->pixels[idx + 2];
        }

        // If the averages differ by enough. Essentially by having used an
        // average of multiple pixels (across the sampling window) instead
        // of comparing individual pixels, we're reducing the effect of
        // random capture noise that's otherwise hard to remove.
        if ((abs(oldR - newR) > threshold) ||
            (abs(oldG - newG) > threshold) ||
            (abs(oldB - newB) > threshold))
        {
            matches++;

            // If we've found that the averages have differed substantially
            // enough times, we conclude that this row of pixels is different from
            // the previous frame, i.e. that it's new data.
            if (matches >= this->matchesRequired)
            {
                return true;
            }
        }

        x += this->stepSize;
    }

    return false;
}

void anti_tearer_c::present_pixels(const u8 *const srcBuffer,
                                   const resolution_s &resolution)
{
    this->presentBuffer.r = resolution;
    std::memcpy(this->presentBuffer.pixels.ptr(), srcBuffer, (resolution.w * resolution.h * (resolution.bpp / 8)));

    return;
}
