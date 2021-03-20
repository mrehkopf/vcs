/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear engine
 *
 * Applies anti-tearing to input frames by detecting which portions of the frame
 * changed since the previous one and accumulating changed portions in a backbuffer
 * until the entire frame has been constructed.
 *
 */

#include <cstring>
#include "filter/anti_tearer.h"
#include "filter/anti_tear.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "common/memory/memory.h"
#include "common/disk/csv.h"

// The color depth we expect frames to be when they're fed into the anti-tear engine.
static const u32 EXPECTED_BIT_DEPTH = 32;

static bool ANTI_TEARING_ENABLED = false;

static anti_tearer_c ANTI_TEARER;

u8* kat_anti_tear(u8 *const pixels, const resolution_s &r)
{
    if (!ANTI_TEARING_ENABLED)
    {
        return pixels;
    }

    return ANTI_TEARER.process(pixels, r);
}

void kat_initialize_anti_tear(void)
{
    const resolution_s &maxres = kc_capture_api().get_maximum_resolution();

    INFO(("Initializing the anti-tear engine for %u x %u max.", maxres.w, maxres.h));

    ANTI_TEARER.initialize({maxres.w, maxres.h, EXPECTED_BIT_DEPTH});

    return;
}

void kat_release_anti_tear(void)
{
    INFO(("Releasing the anti-tear engine."));

    ANTI_TEARER.release();

    return;
}


void kat_set_anti_tear_enabled(const bool state)
{
    ANTI_TEARING_ENABLED = state;

    return;
}

void kat_set_visualization(const bool visualizeTear,
                           const bool visualizeRange)
{
    ANTI_TEARER.visualizeTears = visualizeTear;
    ANTI_TEARER.visualizeScanRange = visualizeRange;

    return;
}

void kat_set_scan_hint(const anti_tear_scan_hint_e newHint)
{
    ANTI_TEARER.scanHint = newHint;

    return;
}

void kat_set_range(const u32 min, const u32 max)
{
    ANTI_TEARER.startRow = min;
    ANTI_TEARER.endRow = max;

    return;
}

void kat_set_threshold(const u32 t)
{
    ANTI_TEARER.threshold = t;

    return;
}

void kat_set_domain_size(const u32 ds)
{
    ANTI_TEARER.windowLength = ds;

    return;
}

void kat_set_step_size(const u32 s)
{
    // A step size of 0 would cause an infinite loop.
    ANTI_TEARER.stepSize = std::max(1u, s);

    return;
}

void kat_set_matches_required(const u32 mr)
{
    ANTI_TEARER.matchesRequired = mr;

    return;
}
