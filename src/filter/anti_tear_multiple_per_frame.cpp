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

void anti_tear_multiple_per_frame_c::process(const captured_frame_s *const frame,
                                             const bool untornFrameAlreadyCopied)
{
    this->prevTearRow = std::min(this->prevTearRow, unsigned(frame->r.h));

    const int firstNewRowIdx = this->base->find_first_new_row_idx(frame, this->base->scanStartRow, this->prevTearRow);
    k_assert((firstNewRowIdx <= int(frame->r.h)), "Tear row overflows frame's height.");

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
            this->base->tornRowIndices.clear();
        }

        this->prevTearRow = frame->r.h;
    }

    return;
}
