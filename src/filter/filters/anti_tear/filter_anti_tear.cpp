/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/anti_tear/filter_anti_tear.h"
#include "anti_tear/anti_tearer.h"

void filter_anti_tear_c::apply(FILTER_APPLY_FUNCTION_PARAMS)
{
    VALIDATE_FILTER_INPUT

    static anti_tearer_c antiTearer;
    static bool isInitialized= false;

    if (!isInitialized)
    {
        antiTearer.initialize({MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT, MAX_CAPTURE_BPP});
        isInitialized = true;
    }

    antiTearer.visualizeScanRange = this->parameter(PARAM_VISUALIZE_RANGE);
    antiTearer.visualizeTears = this->parameter(PARAM_VISUALIZE_TEARS);
    antiTearer.scanStartOffset = this->parameter(PARAM_SCAN_START);
    antiTearer.scanEndOffset = this->parameter(PARAM_SCAN_END);
    antiTearer.threshold = this->parameter(PARAM_THRESHOLD);
    antiTearer.stepSize = this->parameter(PARAM_STEP_SIZE);
    antiTearer.windowLength = this->parameter(PARAM_WINDOW_LENGTH);
    antiTearer.matchesRequired = this->parameter(PARAM_MATCHES_REQD);
    antiTearer.scanDirection = ((this->parameter(PARAM_SCAN_DIRECTION) == SCAN_DOWN)
                                ? anti_tear_scan_direction_e::down
                                : anti_tear_scan_direction_e::up);
    antiTearer.scanHint = ((this->parameter(PARAM_SCAN_HINT) == SCAN_ONE_TEAR)
                           ? anti_tear_scan_hint_e::look_for_one_tear
                           : anti_tear_scan_hint_e::look_for_multiple_tears);

    auto *const processedPixels = antiTearer.process(pixels, r);
    memcpy(pixels, processedPixels, (r.w * r.h * (r.bpp / 8)));

    return;
}
