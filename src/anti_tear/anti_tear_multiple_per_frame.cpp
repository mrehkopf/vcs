/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cstring>
#include <cmath>
#include "anti_tear/anti_tear_frame.h"
#include "anti_tear/anti_tear_one_per_frame.h"
#include "anti_tear/anti_tearer.h"
#include "capture/capture.h"

void anti_tear_multiple_per_frame_c::initialize(anti_tearer_c *const parent)
{
    this->base = parent;
    this->prevTearRow = parent->maximumResolution.h;

    return;
}

void anti_tear_multiple_per_frame_c::process(const anti_tear_frame_s *const frame,
                                             const bool untornFrameAlreadyCopied,
                                             const unsigned recursiveCount)
{
    this->prevTearRow = std::min(this->prevTearRow, this->base->scanEndRow);

    const int firstNewRowIdx = this->base->find_first_new_row_idx(frame, this->base->scanStartRow, this->prevTearRow);
    k_assert((firstNewRowIdx <= int(frame->resolution.h)), "Tear row overflows frame's height.");

    // If we found a tear.
    if (firstNewRowIdx >= 0)
    {
        this->base->tornRowIndices.push_back(firstNewRowIdx);

        this->base->copy_frame_pixel_rows(frame, this->base->backBuffer, firstNewRowIdx, this->prevTearRow);

        // If we've finished reconstructing the frame.
        if (firstNewRowIdx == 0)
        {
            std::swap(this->base->backBuffer, this->base->frontBuffer);
            this->base->tornRowIndices.clear();
            this->prevTearRow = frame->resolution.h;

            // The bottom of the frame might have new pixel data, so scan again.
            if (recursiveCount < 1)
            {
                this->process(frame, true, (recursiveCount + 1));
            }
        }
        else
        {
            this->prevTearRow = firstNewRowIdx;
        }
    }
    // Otherwise, we assume we've reconstructed all tears in this frame, and the
    // frame is ready to be presented.
    else if (!untornFrameAlreadyCopied)
    {
        this->base->copy_frame_pixel_rows(frame, this->base->frontBuffer, 0, frame->resolution.h);
        this->base->tornRowIndices.clear();
        this->prevTearRow = frame->resolution.h;
    }

    return;
}
