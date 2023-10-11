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
#include "anti_tear/anti_tear_frame.h"
#include "anti_tear/anti_tear_one_per_frame.h"
#include "anti_tear/anti_tear_multiple_per_frame.h"

// Default starting parameter values for the anti-tear engine.
#define KAT_DEFAULT_THRESHOLD 3
#define KAT_DEFAULT_WINDOW_LENGTH 8
#define KAT_DEFAULT_NUM_MATCHES_REQUIRED 11
#define KAT_DEFAULT_STEP_SIZE 1
#define KAT_DEFAULT_VISUALIZE_TEARS false
#define KAT_DEFAULT_VISUALIZE_SCAN_RANGE false
#define KAT_DEFAULT_SCAN_DIRECTION anti_tear_scan_direction_e::down
#define KAT_DEFAULT_SCAN_HINT anti_tear_scan_hint_e::look_for_one_tear

// Enumerates the tear-scanning hints which can be given to the anti-tear
// subsystem (see kat_set_scan_hint()). These hints provide insight about the input
// data which may improve the anti-tearer's performance.
enum class anti_tear_scan_hint_e
{
    // Tells the anti-tear subsystem that input images are expected to have at
    // most one tear. In other words, a single un-torn image will span no more
    // than two consecutive input images. This may allow the anti-tearer to apply
    // performance optimizations, but will lead to incorrect anti-tearing results
    // if any input image has more than one tear.
    look_for_one_tear,

    // Tells the anti-tear subsystem that input images may have several tears.
    // In other words, a single un-torn image may span any number of
    // consecutive input images. The anti-tearer may, however, choose to only
    // scan a certain maximum number of consecutive input images before
    // declaring the result to be a fully de-torn image.
    look_for_multiple_tears,
};

// Enumerates the directions in which the anti-tear subsystem can scan images
// for tears.
//
// The scan direction determines which parts of an image following and preceding
// a tear are considered by the anti-tearer to be new or old relative to the
// previous image. For instance, if the scan begins from the top and encounters a
// tear in the middle, the upper half of the image would be considered new;
// whereas the lower half would be considered new if the scan had started from
// the bottom.
//
// For correct anti-tearing results, the scan direction should match the direction
// in which the capture source draws the image (which in turn may depend e.g. on
// the video mode).
enum class anti_tear_scan_direction_e
{
    // Scans input images from bottom to top.
    up,

    // Scans input images from top to bottom.
    down,
};

class anti_tearer_c
{
    friend class anti_tear_one_per_frame_c;
    friend class anti_tear_multiple_per_frame_c;

public:
    void initialize(const resolution_s &maxResolution);

    void release(void);

    // Applies anti-tearing to a copy of the given pixels. Returns a pointer to the copy.
    uint8_t* process(uint8_t *const pixels, const resolution_s &resolution);

    // Anti-tearing parameters.
    unsigned scanStartOffset = 0;
    unsigned scanEndOffset = 0; // Rows from the bottom up, i.e. (height - x).
    anti_tear_scan_direction_e scanDirection = KAT_DEFAULT_SCAN_DIRECTION;
    anti_tear_scan_hint_e scanHint = KAT_DEFAULT_SCAN_HINT;
    unsigned threshold = KAT_DEFAULT_THRESHOLD;
    unsigned stepSize = KAT_DEFAULT_STEP_SIZE;
    unsigned windowLength = KAT_DEFAULT_WINDOW_LENGTH;
    unsigned matchesRequired = KAT_DEFAULT_NUM_MATCHES_REQUIRED;
    bool visualizeTears = KAT_DEFAULT_VISUALIZE_TEARS;
    bool visualizeScanRange = KAT_DEFAULT_VISUALIZE_SCAN_RANGE;

protected:
    // Copies the front buffer's pixels into the present buffer and returns a pointer
    // to the present buffer.
    uint8_t* present_front_buffer(const resolution_s &resolution);

    void copy_frame_pixel_rows(const anti_tear_frame_s *const srcFrame,
                               uint8_t *const dstBuffer,
                               const unsigned fromRow,
                               const unsigned toRow);

    // Scans the given frame's rowIdx'th row to find whether the row's pixels have
    // chaned since the last full frame.
    bool has_pixel_row_changed(const unsigned rowIdx,
                               const uint8_t *const newPixels,
                               const uint8_t *const prevPixels,
                               const resolution_s &resolution);

    // Scans the given frame for a tear within the given row range. If a tear is found,
    // returns the index of the pixel row on which the tear starts. Otherwise, returns
    // -1.
    int find_first_new_row_idx(const anti_tear_frame_s *const frame,
                               const unsigned scanStartOffset,
                               const unsigned scanEndOffset);

    // Draws a visual indicator of the current scan range into the given frame.
    void visualize_scan_range(const anti_tear_frame_s &frame);

    // Draws a visual indicator of the most recently found tears into the given frame.
    void visualize_tears(const anti_tear_frame_s &frame);

    anti_tear_one_per_frame_c onePerFrame;
    anti_tear_multiple_per_frame_c multiplePerFrame;

    // Buffers for constructing whole frames from torn ones. The back buffer
    // accumulates torn frame fragments, and the front buffer stores the latest
    // fully reconstructed frame.
    uint8_t *buffers[2];
    uint8_t *backBuffer = nullptr;
    uint8_t *frontBuffer = nullptr;

    // Holds the pixels the anti-tearer considers ready for display; e.g.
    // the latest de-torn frame. Note that this image might hold additional
    // things, like anti-tear parameter visualization.
    anti_tear_frame_s presentBuffer;

    // The maximum size of frames that we can anti-tear.
    resolution_s maximumResolution;

    // The frame row locations of the most recent tears. Used for e.g. visualization.
    std::vector<unsigned> tornRowIndices;

    // The most recent vertical range over which a frame was scanned for tears.
    unsigned scanStartRow = 0;
    unsigned scanEndRow = 0;
};

#endif
