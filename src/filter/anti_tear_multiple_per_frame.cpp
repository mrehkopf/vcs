/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <cstring>
#include <cmath>
#include "filter/anti_tear_one_per_frame.h"
#include "filter/anti_tearer.h"
#include "capture/capture.h"

void anti_tear_multiple_per_frame_c::initialize(anti_tearer_c *const parent)
{
    this->base = parent;
    this->prevTearRow = parent->maximumResolution.h;

    return;
}

void anti_tear_multiple_per_frame_c::present_front_buffer(const resolution_s &resolution)
{
    this->base->present_pixels(this->base->frontBuffer, resolution);

    this->visualize_scan_range();
    this->visualize_tears();

    this->tornRows.clear();
    
    return;
}

void anti_tear_multiple_per_frame_c::process(const captured_frame_s *const frame,
                                             const bool untornFrameAlreadyCopied)
{
    this->prevTearRow = std::min(this->prevTearRow, unsigned(frame->r.h));
    this->scanEndRow = std::max(0ul, (frame->r.h - this->base->endRow));
    this->scanStartRow = std::min(this->base->startRow, this->scanEndRow);

    const int firstNewRowIdx = this->find_first_new_row_idx(frame, this->scanStartRow, this->prevTearRow);
    k_assert((firstNewRowIdx <= int(frame->r.h)), "Tear row overflows frame's height.");

    // If we found a tear.
    if (firstNewRowIdx >= 0)
    {
        this->tornRows.push_back(firstNewRowIdx);

        this->base->copy_frame_pixel_rows(frame, this->base->backBuffer, firstNewRowIdx, this->prevTearRow);

        // If we've finished reconstructing the frame.
        if (firstNewRowIdx == 0)
        {
            std::swap(this->base->backBuffer, this->base->frontBuffer);
            this->present_front_buffer(frame->r);

            this->prevTearRow = frame->r.h;

            // The bottom of the frame might have new pixel data, so keep scanning.
            this->process(frame, true);
        }
        else
        {
            this->prevTearRow = firstNewRowIdx;
        }
    }
    // Otherwise, we assume we've reconstructed all tears in this frame, and the
    // frame is ready to be presented.
    else
    {
        if (!untornFrameAlreadyCopied)
        {
            this->base->copy_frame_pixel_rows(frame, this->base->frontBuffer, 0, frame->r.h);
            this->present_front_buffer(frame->r);
        }

        this->prevTearRow = frame->r.h;
    }

    return;
}

int anti_tear_multiple_per_frame_c::find_first_new_row_idx(const captured_frame_s *frame,
                                                           const unsigned startRow,
                                                           const unsigned endRow)
{
    for (unsigned rowIdx = startRow; rowIdx < endRow; rowIdx++)
    {
        if (this->base->has_pixel_row_changed(rowIdx, frame->pixels.ptr(), this->base->frontBuffer, frame->r))
        {
            // If the new row of pixels is at the top of the frame, there's no
            // tearing (we assume the frame fills in from bottom to top).
            return ((rowIdx == startRow)? -1 : rowIdx);
        }
    }

    return -1;
}

void anti_tear_multiple_per_frame_c::visualize_tears(void)
{
    if (!this->base->visualizeTears)
    {
        return;
    }

    captured_frame_s &frame = this->base->presentBuffer;

    for (const auto &tornRow:this->tornRows)
    {
        const unsigned bpp = (frame.r.bpp / 8);
        const unsigned idx = (tornRow * frame.r.w * bpp);
        std::memset((this->base->presentBuffer.pixels.ptr() + idx), 255, (frame.r.w * bpp));
    }

    return;
}

void anti_tear_multiple_per_frame_c::visualize_scan_range(void)
{
    if (!this->base->visualizeScanRange)
    {
        return;
    }

    const unsigned patternDensity = 9;

    captured_frame_s &frame = this->base->presentBuffer;

    // Shade the area under the scan range.
    for (unsigned y = this->scanStartRow; y < this->scanEndRow; y++)
    {
        for (unsigned x = 0; x < frame.r.w; x++)
        {
            const unsigned idx = ((x + y * frame.r.w) * 4);

            frame.pixels[idx + 1] *= 0.5;
            frame.pixels[idx + 2] *= 0.5;

            // Create a dot pattern.
            if (((y % patternDensity) == 0) &&
                ((x + y) % (patternDensity * 2)) == 0)
            {
                frame.pixels[idx + 0] = ~frame.pixels[idx + 0];
                frame.pixels[idx + 1] = ~frame.pixels[idx + 1];
                frame.pixels[idx + 2] = ~frame.pixels[idx + 2];
            }
        }
    }

    // Indicate with a line where the scan range starts and ends.
    for (unsigned x = 0; x < frame.r.w; x++)
    {
        if (((x / patternDensity) % 2) == 0)
        {
            int idx = ((x + this->scanStartRow * frame.r.w) * 4);
            frame.pixels[idx + 0] = ~frame.pixels[idx + 0];
            frame.pixels[idx + 1] = ~frame.pixels[idx + 1];
            frame.pixels[idx + 2] = ~frame.pixels[idx + 2];

            idx = ((x + (this->scanEndRow - 1) * frame.r.w) * 4);
            frame.pixels[idx + 0] = ~frame.pixels[idx + 0];
            frame.pixels[idx + 1] = ~frame.pixels[idx + 1];
            frame.pixels[idx + 2] = ~frame.pixels[idx + 2];
        }
    }

    return;
}
