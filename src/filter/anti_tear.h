/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_H
#define VCS_FILTER_ANTI_TEAR_H

#include "common/globals.h"

struct captured_frame_s;
struct resolution_s;

struct frame_tears_s
{
    u32 numTears;       // How many tears we have.
    u32 tearY[2];       // We expect a maximum of 2 tears.
    bool newData[2];
};

struct tear_frame_s
{
    u8 *pixels;         // Pointer to the start of this frame's pixels in the master buffer.
    u32 newDataStart;   // The y height up to which this frame has been filled with new data.
    bool isDone;        // Set to true once this frame's reconstruction has been completed.
};

struct anti_tear_options_s
{
    real threshold;
    u32 scanStart;
    u32 scanEnd;
    u32 windowLen;
    u32 stepSize;
    u32 matchesReqd;
};

void kat_initialize_anti_tear(void);

void kat_set_buffer_updates_disabled(const bool disabled);

void kat_release_anti_tear(void);

const anti_tear_options_s& kat_default_settings(void);

void kat_set_range(const u32 min, const u32 max);

u8 *kat_anti_tear(u8 *const pixels, const resolution_s &r);

void kat_set_anti_tear_enabled(const bool state);

bool kat_is_anti_tear_enabled(void);

void kat_set_visualization(const bool visualize, const bool visualizeTear, const bool visualizeRange);

void kat_set_threshold(const u32 t);

void kat_set_domain_size(const u32 ds);

void kat_set_step_size(const u32 s);

void kat_set_matches_required(const u32 mr);

#endif
