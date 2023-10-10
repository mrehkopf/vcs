/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/anti_tear/filter_anti_tear.h"
#include "anti_tear/anti_tearer.h"

void filter_anti_tear_c::apply(image_s *const image)
{
    ASSERT_FILTER_ARGUMENTS(image);

    static anti_tearer_c antiTearer;
    static bool isInitialized= false;

    if (!isInitialized)
    {
        antiTearer.initialize(resolution_s{.w = MAX_CAPTURE_WIDTH, .h = MAX_CAPTURE_HEIGHT});
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

    const uint8_t *const processedPixels = antiTearer.process(image->pixels, image->resolution);
    memcpy(image->pixels, processedPixels, (image->resolution.w * image->resolution.h * (image->bitsPerPixel / 8)));

    return;
}
