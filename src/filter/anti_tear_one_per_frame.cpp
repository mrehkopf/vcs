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

void anti_tear_one_per_frame_c::initialize(anti_tearer_c *const parent)
{
    this->base = parent;

    return;
}

void anti_tear_one_per_frame_c::present_pixels(u8 *const srcBuffer,
                                               const resolution_s &resolution,
                                               const bool visualizeScanRange)
{
    this->base->present_pixels(srcBuffer, resolution);

    if (visualizeScanRange)
    {
        this->visualize_scan_range();
    }

    return;
}

void anti_tear_one_per_frame_c::process(const captured_frame_s *const frame,
                                        const bool visualizeTear,
                                        const bool visualizeScanRange)
{
    switch (this->nextAction)
    {
        // If we found a tear last frame, finish copying its data from this new frame
        // to generate a finished front buffer.
        case next_action_e::copy_rest_of_pixel_data:
        {
            this->base->copy_frame_pixel_rows(this->base->backBuffer, frame, 0, this->latestTearRow);

            // The back buffer now holds (we assume) a finished, un-torn frame. So we
            // can swap it for the front buffer and present its contents.
            std::swap(this->base->backBuffer, this->base->frontBuffer);
            this->present_pixels(this->base->frontBuffer, frame->r, visualizeScanRange);

            if (visualizeTear && (this->latestTearRow >= 0))
            {
                const unsigned bpp = (frame->r.bpp / 8);
                const unsigned idx = (this->latestTearRow * frame->r.w * bpp);
                std::memset((this->base->presentBuffer.pixels.ptr() + idx), 255, (frame->r.w * bpp));
            }

            this->nextAction = next_action_e::scan_for_tear;
            this->latestTearRow = -1;

            // Fall through to scan for the next tear.
            __attribute__ ((fallthrough));
        }
        // Look for a new tear and populate the back buffer with its data.
        case next_action_e::scan_for_tear:
        {
            this->scanEndRow = std::max(0ul, (frame->r.h - this->base->endRow));
            this->scanStartRow = std::min(this->base->startRow, this->scanEndRow);
            this->latestTearRow = this->find_tear_row(frame, this->scanStartRow, this->scanEndRow);

            k_assert((this->latestTearRow <= int(frame->r.h)), "Tear row overflows frame's height.");

            // If we found a tear.
            if (this->latestTearRow >= 0)
            {
                this->base->copy_frame_pixel_rows(this->base->backBuffer, frame, this->latestTearRow, frame->r.h);
                this->nextAction = next_action_e::copy_rest_of_pixel_data;
            }
            // Otherwise, we assume the frame is not torn and is suitable for display.
            else
            {
                this->base->copy_frame_pixel_rows(this->base->frontBuffer, frame, 0, frame->r.h);
                this->present_pixels(this->base->frontBuffer, frame->r, visualizeScanRange);
            }

            break;
        }
    }

    return;
}

int anti_tear_one_per_frame_c::find_tear_row(const captured_frame_s *const frame,
                                             unsigned startRow,
                                             unsigned endRow)
{
    k_assert((endRow <= frame->r.h), "Invalid arguments.");

    // The image is expected to be filled in from bottom to top, so if
    // the first row is new, there can be no tear.
    if (this->base->is_pixel_row_new(startRow, frame))
    {
        return -1;
    }

    int firstNewRow = frame->r.h;
    int prevRow = startRow;
    int rowDelta = std::floor((endRow - startRow) / 2);

    // Scan the frame's horizontal pixel rows by iteratively splitting the
    // vertical range in the direction of the tear.
    for (int curRow = (startRow + rowDelta); std::abs(rowDelta) > 0; curRow += rowDelta)
    {
        const bool isNewRow = this->base->is_pixel_row_new(curRow, frame);
        firstNewRow = (isNewRow? curRow : firstNewRow);
        rowDelta = (std::floor(std::abs(curRow - prevRow) / 2) * (isNewRow? -1 : 1));
        prevRow = curRow;
    }

    return ((unsigned(firstNewRow) >= frame->r.h)? -1 : firstNewRow);
}

void anti_tear_one_per_frame_c::visualize_scan_range(void)
{
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
