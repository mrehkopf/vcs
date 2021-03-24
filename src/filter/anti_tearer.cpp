/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <cstring>
#include "filter/anti_tearer.h"
#include "filter/anti_tear_frame.h"

void anti_tearer_c::release(void)
{
    this->backBuffer = nullptr;
    this->frontBuffer = nullptr;

    this->buffers[0].release_memory();
    this->buffers[1].release_memory();
    this->presentBuffer.pixels.release_memory();

    return;
}

void anti_tearer_c::initialize(const resolution_s &maxResolution)
{
    const unsigned requiredBufferSize = maxResolution.w * maxResolution.h * (maxResolution.bpp / 8);

    this->maximumResolution = maxResolution;

    this->buffers[0].alloc(requiredBufferSize, "Anti-tearing buffer #1");
    this->buffers[1].alloc(requiredBufferSize, "Anti-tearing buffer #2");
    this->presentBuffer.pixels.alloc(requiredBufferSize, "Anti-tearing present buffer.");

    this->backBuffer = this->buffers[0].ptr();
    this->frontBuffer = this->buffers[1].ptr();
    this->presentBuffer.resolution = {0, 0, 32};

    this->onePerFrame.initialize(this);
    this->multiplePerFrame.initialize(this);

    return;
}

u8* anti_tearer_c::process(u8 *const pixels,
                           const resolution_s &resolution)
{
    k_assert((pixels != nullptr),
             "The anti-tear engine expected a pixel buffer, but received null.");

    k_assert((resolution.bpp == this->presentBuffer.resolution.bpp),
             "The anti-tear engine expected a certain bit depth, but the input frame did not comply.");

    k_assert((resolution.w <= this->maximumResolution.w) &&
             (resolution.h <= this->maximumResolution.h),
             "The frame is too large to apply anti-tearing.");

    anti_tear_frame_s frame(resolution, pixels);

    this->scanEndRow = std::max(0ul, (frame.resolution.h - this->scanEndOffset));
    this->scanStartRow = std::min(this->scanStartOffset, this->scanEndRow);

    if (this->scanDirection == anti_tear_scan_direction_e::up)
    {
        frame.flip_vertically();
    }

    switch (this->scanHint)
    {
        case anti_tear_scan_hint_e::look_for_multiple_tears: this->multiplePerFrame.process(&frame); break;
        case anti_tear_scan_hint_e::look_for_one_tear: this->onePerFrame.process(&frame); break;
    }

    return this->present_front_buffer(frame.resolution);
}

u8* anti_tearer_c::present_front_buffer(const resolution_s &resolution)
{
    this->presentBuffer.resolution = resolution;

    std::memcpy(this->presentBuffer.pixels.ptr(),
                this->frontBuffer,
                this->presentBuffer.pixels.up_to(resolution.w * resolution.h * (resolution.bpp / 8)));

    if (this->visualizeScanRange)
    {
        this->visualize_scan_range(this->presentBuffer);
    }

    if (this->visualizeTears)
    {
        this->visualize_tears(this->presentBuffer);
    }

    // If we were scanning upwards, the frame was flipped for processing, so we
    // unflip it here for display.
    if (this->scanDirection == anti_tear_scan_direction_e::up)
    {
        this->presentBuffer.flip_vertically();
    }

    return this->presentBuffer.pixels.ptr();
}

void anti_tearer_c::visualize_tears(const anti_tear_frame_s &frame)
{
    for (const auto &tornRow:this->tornRowIndices)
    {
        const unsigned bpp = (frame.resolution.bpp / 8);
        const unsigned idx = (tornRow * frame.resolution.w * bpp);
        std::memset((frame.pixels.ptr() + idx), 255, (frame.resolution.w * bpp));
    }

    return;
}

void anti_tearer_c::visualize_scan_range(const anti_tear_frame_s &frame)
{
    const unsigned patternDensity = 9;

    // Shade the area under the scan range.
    for (unsigned y = this->scanStartRow; y < this->scanEndRow; y++)
    {
        for (unsigned x = 0; x < frame.resolution.w; x++)
        {
            const unsigned idx = ((x + y * frame.resolution.w) * 4);

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
    for (unsigned x = 0; x < frame.resolution.w; x++)
    {
        if (((x / patternDensity) % 2) == 0)
        {
            int idx = ((x + this->scanStartRow * frame.resolution.w) * 4);
            frame.pixels[idx + 0] = ~frame.pixels[idx + 0];
            frame.pixels[idx + 1] = ~frame.pixels[idx + 1];
            frame.pixels[idx + 2] = ~frame.pixels[idx + 2];

            idx = ((x + (this->scanEndRow - 1) * frame.resolution.w) * 4);
            frame.pixels[idx + 0] = ~frame.pixels[idx + 0];
            frame.pixels[idx + 1] = ~frame.pixels[idx + 1];
            frame.pixels[idx + 2] = ~frame.pixels[idx + 2];
        }
    }

    return;
}

void anti_tearer_c::copy_frame_pixel_rows(const anti_tear_frame_s *const srcFrame,
                                          u8 *const dstBuffer,
                                          const unsigned fromRow,
                                          const unsigned toRow)
{
    if (fromRow == toRow)
    {
        return;
    }

    k_assert(((fromRow < toRow) && (toRow <= srcFrame->resolution.h)), "Malformed parameters.");

    const unsigned bpp = (srcFrame->resolution.bpp / 8);
    const unsigned idx = ((fromRow * srcFrame->resolution.w) * bpp);
    const unsigned numBytes = (((toRow - fromRow) * srcFrame->resolution.w) * bpp);
    std::memcpy((dstBuffer + idx), (srcFrame->pixels.ptr() + idx), numBytes);

    return;
}

int anti_tearer_c::find_first_new_row_idx(const anti_tear_frame_s *const frame,
                                          const unsigned startRow,
                                          const unsigned endRow)
{
    for (unsigned rowIdx = startRow; rowIdx < endRow; rowIdx++)
    {
        if (this->has_pixel_row_changed(rowIdx, frame->pixels.ptr(), this->frontBuffer, frame->resolution))
        {
            // If the new row of pixels is at the top of the frame, there's no
            // tearing (we assume the frame fills in from bottom to top).
            return ((rowIdx == startRow)? -1 : rowIdx);
        }
    }

    return -1;
}

bool anti_tearer_c::has_pixel_row_changed(const unsigned rowIdx,
                                          const u8 *const newPixels,
                                          const u8 *const prevPixels,
                                          const resolution_s &resolution)
{
    k_assert((newPixels && prevPixels), "Expected non-null pixel data.");

    k_assert((rowIdx < resolution.h), "Row index overflowing the pixel data.");

    unsigned x = 0;
    unsigned matches = 0;
    const unsigned threshold = (this->windowLength * this->threshold);

    // Slide a sampling window across this horizontal row of pixels.
    while ((x + this->windowLength) < resolution.w)
    {
        int oldR = 0, oldG = 0, oldB = 0;
        int newR = 0, newG = 0, newB = 0;

        // Find the average color values of the current and the previous frame
        // within this sampling window.
        for (size_t w = 0; w < this->windowLength; w++)
        {
            const unsigned bpp = (resolution.bpp / 8);
            const unsigned idx = (((x + w) + rowIdx * resolution.w) * bpp);

            oldB += prevPixels[idx + 0];
            oldG += prevPixels[idx + 1];
            oldR += prevPixels[idx + 2];

            newB += newPixels[idx + 0];
            newG += newPixels[idx + 1];
            newR += newPixels[idx + 2];
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
