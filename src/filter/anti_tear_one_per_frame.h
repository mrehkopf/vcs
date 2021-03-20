/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_ONE_PER_FRAME_H
#define VCS_FILTER_ANTI_TEAR_ONE_PER_FRAME_H

#include "common/types.h"

struct captured_frame_s;
struct resolution_s;
class anti_tearer_c;

// Applies anti-tearing to a given frame (via process()). Assumes that to create
// a non-torn frame, data from at most two consecutive torn frames will be needed;
// e.g. that there will be only up to one tear per full frame.
class anti_tear_one_per_frame_c
{
public:
    void initialize(anti_tearer_c *const base);

    void process(const captured_frame_s *const frame);

private:
    void present_front_buffer(const resolution_s &resolution, const bool visualizeTear, const bool visualizeScanRange);

    void visualize_scan_range(void);

    void visualize_current_tear(void);

    // Scans the given frame for a tear. If a tear is found, returns the pixel row
    // on which it starts. Otherwise, returns -1.
    int find_first_new_row_idx(const captured_frame_s *frame);

    enum class next_action_e
    {
        scan_for_tear,
        copy_rest_of_pixel_data,
    } nextAction = next_action_e::scan_for_tear;

    int latestTearRow = -1;

    // The range of the latest tear scan we've done.
    unsigned scanStartRow = 0;
    unsigned scanEndRow = 0;

    anti_tearer_c *base;
};

#endif
