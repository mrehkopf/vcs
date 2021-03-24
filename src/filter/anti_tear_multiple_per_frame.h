/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_MULTIPLE_PER_FRAME_H
#define VCS_FILTER_ANTI_TEAR_MULTIPLE_PER_FRAME_H

struct anti_tear_frame_s;
struct resolution_s;
class anti_tearer_c;

// Applies anti-tearing to a given frame (via process()). Assumes that to create a
// non-torn frame, data from more than two consecutive torn frames may be needed;
// e.g. that there may be two or more tears per full frame.
class anti_tear_multiple_per_frame_c
{
public:
    void initialize(anti_tearer_c *const base);

    void process(const anti_tear_frame_s *const frame,
                 const bool untornFrameAlreadyCopied = false,
                 const unsigned recursiveCount = 0);

private:
    unsigned prevTearRow = 0;

    anti_tearer_c *base;
};

#endif
