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
#include "filter/anti_tear_one_per_frame.h"

class anti_tearer_c
{
    friend class anti_tear_one_per_frame_c;

public:
    virtual ~anti_tearer_c(void);
    
    void initialize(const resolution_s &maxResolution);

    // Returns the present buffer's pixels.
    u8* pixels(void) const;

    anti_tear_one_per_frame_c onePerFrame;

    // Anti-tearing parameters.
    unsigned startRow = 0;
    unsigned endRow = 0; // Rows from the bottom up, i.e. (height - x).
    unsigned threshold = 3;
    unsigned stepSize = 1;
    unsigned windowLength = 8;
    unsigned matchesRequired = 11;

protected:
    void copy_frame_pixel_rows(u8 *const dstBuffer,
                               const captured_frame_s *const srcFrame,
                               const unsigned fromRow,
                               const unsigned toRow);

    // Scans the given frame for a tear. If a tear is found, returns the pixel row
    // on which it starts. Otherwise, returns -1.
    int find_tear_row(const captured_frame_s *const frame,
                      unsigned startY,
                      unsigned endY);

    // Scans the given frame's rowIdx'th row to find whether the row's pixels have
    // chaned since the last full frame.
    bool is_pixel_row_new(const unsigned rowIdx,
                          const captured_frame_s *const frame);

    // Copies the given buffer's pixels into the present buffer.
    void present_pixels(const u8 *const srcBuffer,
                        const resolution_s &resolution);

    // We'll use these buffers to accumulate torn pixel data and so to reconstruct
    // torn frames.
    std::vector<heap_bytes_s<u8>> buffers = std::vector<heap_bytes_s<u8>>(3);

    // Buffers for constructing whole frames from partial torn ones. For
    // instance, the back buffer might be where torn frames are accumulates,
    // and the front buffer the latest full frame.
    u8 *backBuffer = nullptr;
    u8 *frontBuffer = nullptr;

    // Holds the pixels the anti-tearer considers ready for display; e.g.
    // the latest de-torn frame. Note that this image might hold additional
    // things, like anti-tear parameter visualization.
    captured_frame_s presentBuffer;
};

#endif
