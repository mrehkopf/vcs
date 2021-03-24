/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_H
#define VCS_FILTER_ANTI_TEAR_H

#include "common/globals.h"

// Default starting parameter values for the anti-tear engine.
#define KAT_DEFAULT_THRESHOLD 3
#define KAT_DEFAULT_WINDOW_LENGTH 8
#define KAT_DEFAULT_NUM_MATCHES_REQUIRED 11
#define KAT_DEFAULT_STEP_SIZE 1
#define KAT_DEFAULT_VISUALIZE_TEARS false
#define KAT_DEFAULT_VISUALIZE_SCAN_RANGE false
#define KAT_DEFAULT_SCAN_DIRECTION anti_tear_scan_direction_e::down
#define KAT_DEFAULT_SCAN_HINT anti_tear_scan_hint_e::look_for_one_tear

struct resolution_s;

enum class anti_tear_scan_hint_e
{
    look_for_one_tear,
    look_for_multiple_tears,
};

enum class anti_tear_scan_direction_e
{
    up,
    down,
};

void kat_initialize_anti_tear(void);

void kat_release_anti_tear(void);

void kat_set_scan_hint(const anti_tear_scan_hint_e newHint);

void kat_set_scan_direction(const anti_tear_scan_direction_e newDirection);

void kat_set_range(const u32 min, const u32 max);

u8 *kat_anti_tear(u8 *const pixels, const resolution_s &r);

void kat_set_anti_tear_enabled(const bool state);

void kat_set_visualization(const bool visualizeTear, const bool visualizeRange);

void kat_set_threshold(const u32 t);

void kat_set_domain_size(const u32 ds);

void kat_set_step_size(const u32 s);

void kat_set_matches_required(const u32 mr);

#endif
