/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEARER_H
#define VCS_FILTER_ANTI_TEARER_H

#include <vector>
#include "capture/capture.h"
#include "common/types.h"
#include "filter/anti_tear.h"
#include "filter/anti_tear_one_per_frame.h"
#include "filter/anti_tear_multiple_per_frame.h"

class anti_tearer_c
{
    friend class anti_tear_one_per_frame_c;
    friend class anti_tear_multiple_per_frame_c;

public:
    void initialize(const resolution_s &maxResolution);

    void release(void);

    // Applies anti-tearing to a copy of the given pixels. Returns a pointer to the copy.
    u8* process(u8 *const pixels,
                const resolution_s &resolution);

    // Anti-tearing parameters.
    unsigned startRow = 0;
    unsigned endRow = 0; // Rows from the bottom up, i.e. (height - x).
    unsigned threshold = KAT_DEFAULT_THRESHOLD;
    unsigned stepSize = KAT_DEFAULT_STEP_SIZE;
    unsigned windowLength = KAT_DEFAULT_WINDOW_LENGTH;
    unsigned matchesRequired = KAT_DEFAULT_NUM_MATCHES_REQUIRED;
    bool visualizeTears = KAT_DEFAULT_VISUALIZE_TEARS;
    bool visualizeScanRange = KAT_DEFAULT_VISUALIZE_SCAN_RANGE;
    anti_tear_scan_hint_e scanHint = anti_tear_scan_hint_e::look_for_one_tear;

protected:
    void copy_frame_pixel_rows(const captured_frame_s *const srcFrame,
                               u8 *const dstBuffer,
                               const unsigned fromRow,
                               const unsigned toRow);

    // Scans the given frame's rowIdx'th row to find whether the row's pixels have
    // chaned since the last full frame.
    bool has_pixel_row_changed(const unsigned rowIdx,
                               const u8 *const newPixels,
                               const u8 *const prevPixels,
                               const resolution_s &resolution);

    // Copies the given buffer's pixels into the present buffer.
    void present_pixels(const u8 *const srcBuffer,
                        const resolution_s &resolution);

    anti_tear_one_per_frame_c onePerFrame;
    anti_tear_multiple_per_frame_c multiplePerFrame;

    // We'll use these buffers to accumulate torn pixel data and so to reconstruct
    // torn frames.
    std::vector<heap_bytes_s<u8>> buffers = std::vector<heap_bytes_s<u8>>(2);

    // Buffers for constructing whole frames from partial torn ones. For
    // instance, the back buffer might be where torn frames are accumulates,
    // and the front buffer the latest full frame.
    u8 *backBuffer = nullptr;
    u8 *frontBuffer = nullptr;

    // Holds the pixels the anti-tearer considers ready for display; e.g.
    // the latest de-torn frame. Note that this image might hold additional
    // things, like anti-tear parameter visualization.
    captured_frame_s presentBuffer;

    // The maximum size of frames that we can anti-tear.
    resolution_s maximumResolution;
};

#endif
