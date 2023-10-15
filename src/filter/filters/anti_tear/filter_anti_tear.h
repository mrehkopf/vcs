/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_ANTI_TEAR_FILTER_ANTI_TEAR_H
#define VCS_FILTER_FILTERS_ANTI_TEAR_FILTER_ANTI_TEAR_H

#include "anti_tear/anti_tearer.h"
#include "filter/abstract_filter.h"
#include "anti_tear/anti_tearer.h"

class filter_anti_tear_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_anti_tear_c)

    enum { PARAM_SCAN_DIRECTION,
           PARAM_SCAN_HINT,
           PARAM_SCAN_START,
           PARAM_SCAN_END,
           PARAM_THRESHOLD,
           PARAM_WINDOW_LENGTH,
           PARAM_STEP_SIZE,
           PARAM_MATCHES_REQD,
           PARAM_VISUALIZE_TEARS,
           PARAM_VISUALIZE_RANGE };

    enum { SCAN_DOWN = 0,
           SCAN_UP = 1, };

    enum { SCAN_ONE_TEAR = 0,
           SCAN_MULTIPLE_TEARS = 1 };

    filter_anti_tear_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_SCAN_DIRECTION, SCAN_DOWN},
            {PARAM_SCAN_HINT, SCAN_MULTIPLE_TEARS},
            {PARAM_SCAN_START, 0},
            {PARAM_SCAN_END, 0},
            {PARAM_THRESHOLD, KAT_DEFAULT_THRESHOLD},
            {PARAM_WINDOW_LENGTH, KAT_DEFAULT_WINDOW_LENGTH},
            {PARAM_STEP_SIZE, KAT_DEFAULT_STEP_SIZE},
            {PARAM_MATCHES_REQD, KAT_DEFAULT_NUM_MATCHES_REQUIRED},
            {PARAM_VISUALIZE_TEARS, 0},
            {PARAM_VISUALIZE_RANGE, 0}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *scanDir = filtergui::combo_box(filter, PARAM_SCAN_DIRECTION);
            scanDir->items = {"Down", "Up"};
            gui->fields.push_back({"Scan direction", {scanDir}});

            auto *scanHint = filtergui::combo_box(filter, PARAM_SCAN_HINT);
            scanHint->items = {"Up to one tear", "Multiple tears"};
            gui->fields.push_back({"Scan hint", {scanHint}});

            auto *scanStart = filtergui::spinner(filter, PARAM_SCAN_START);
            scanStart->minValue = 0;
            scanStart->maxValue = MAX_CAPTURE_HEIGHT;
            auto *scanEnd = filtergui::spinner(filter, PARAM_SCAN_END);
            scanEnd->minValue = 0;
            scanEnd->maxValue = MAX_CAPTURE_HEIGHT;
            gui->fields.push_back({"Scan range", {scanStart, scanEnd}});

            auto *threshold = filtergui::spinner(filter, PARAM_THRESHOLD);
            threshold->minValue = 0;
            threshold->maxValue = 255;
            gui->fields.push_back({"Threshold", {threshold}});

            auto *windowSize = filtergui::spinner(filter, PARAM_WINDOW_LENGTH);
            windowSize->minValue = 1;
            windowSize->maxValue = 99;
            gui->fields.push_back({"Window size", {windowSize}});

            auto *stepSize = filtergui::spinner(filter, PARAM_STEP_SIZE);
            stepSize->minValue = 1;
            stepSize->maxValue = 99;
            gui->fields.push_back({"Step size", {stepSize}});

            auto *matches = filtergui::spinner(filter, PARAM_MATCHES_REQD);
            matches->minValue = 1;
            matches->maxValue = 255;
            gui->fields.push_back({"Matches req'd", {matches}});

            auto *tears = filtergui::checkbox(filter, PARAM_VISUALIZE_TEARS);
            tears->label = "Tears";
            auto *range = filtergui::checkbox(filter, PARAM_VISUALIZE_RANGE);
            range->label = "Range";
            gui->fields.push_back({"Visualize", {tears, range}});
        });
    }

    void apply(image_s *const image) override
    {
        static anti_tearer_c antiTearer;
        static bool isInitialized = false;

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
        antiTearer.scanDirection = (
            (this->parameter(PARAM_SCAN_DIRECTION) == SCAN_DOWN)
                ? anti_tear_scan_direction_e::down
                : anti_tear_scan_direction_e::up
        );
        antiTearer.scanHint = (
            (this->parameter(PARAM_SCAN_HINT) == SCAN_ONE_TEAR)
                ? anti_tear_scan_hint_e::look_for_one_tear
                : anti_tear_scan_hint_e::look_for_multiple_tears
        );

        const uint8_t *const processedPixels = antiTearer.process(image->pixels, image->resolution);
        memcpy(image->pixels, processedPixels, (image->resolution.w * image->resolution.h * (image->bitsPerPixel / 8)));

        return;
    }

    std::string uuid(void) const override { return "11c27e0a-a000-41e9-a134-7579073c7dc5"; }
    std::string name(void) const override { return "Anti-tear"; }
    filter_category_e category(void) const override { return filter_category_e::enhance; }

private:
};

#endif
