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

// Applies anti-tearing on a given frame with the assumption that each
// frame can have at most one tear.
class anti_tear_one_per_frame_c
{
public:
    void initialize(anti_tearer_c *const base);

    void process(const captured_frame_s *const frame, const bool visualizeTear,
                 const bool visualizeScanRange);

    void visualize_scan_range(void);

private:
    void present_pixels(u8 *const srcBuffer,
                        const resolution_s &resolution,
                        const bool visualizeScanRange);

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
