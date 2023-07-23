/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/anti_tear/filter_anti_tear.h"
#include "filter/filters/anti_tear/gui/filtergui_anti_tear.h"

filtergui_anti_tear_c::filtergui_anti_tear_c(abstract_filter_c *const filter)
{
    {
        auto *const scanDir = new abstract_gui::combo_box;

        scanDir->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_SCAN_DIRECTION);};
        scanDir->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_SCAN_DIRECTION, value);};
        scanDir->items = {"Down", "Up"};

        this->fields.push_back({"Scan direction", {scanDir}});
    }

    {
        auto *const scanHint = new abstract_gui::combo_box;

        scanHint->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_SCAN_HINT);};
        scanHint->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_SCAN_HINT, value);};
        scanHint->items = {"Up to one tear", "Multiple tears"};

        this->fields.push_back({"Scan hint", {scanHint}});
    }

    {
        auto *const scanStart = new abstract_gui::spinner;
        scanStart->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_SCAN_START);};
        scanStart->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_SCAN_START, value);};
        scanStart->minValue = 0;
        scanStart->maxValue = MAX_CAPTURE_HEIGHT;

        auto *const scanEnd = new abstract_gui::spinner;
        scanEnd->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_SCAN_END);};
        scanEnd->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_SCAN_END, value);};
        scanEnd->minValue = 0;
        scanEnd->maxValue = MAX_CAPTURE_HEIGHT;

        this->fields.push_back({"Scan range", {scanStart, scanEnd}});
    }

    {
        auto *const threshold = new abstract_gui::spinner;

        threshold->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_THRESHOLD);};
        threshold->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_THRESHOLD, value);};
        threshold->minValue = 0;
        threshold->maxValue = 255;

        this->fields.push_back({"Threshold", {threshold}});
    }

    {
        auto *const windowSize = new abstract_gui::spinner;

        windowSize->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_WINDOW_LENGTH);};
        windowSize->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_WINDOW_LENGTH, value);};
        windowSize->minValue = 1;
        windowSize->maxValue = 99;

        this->fields.push_back({"Window size", {windowSize}});
    }

    {
        auto *const stepSize = new abstract_gui::spinner;

        stepSize->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_STEP_SIZE);};
        stepSize->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_STEP_SIZE, value);};
        stepSize->minValue = 1;
        stepSize->maxValue = 99;

        this->fields.push_back({"Step size", {stepSize}});
    }

    {
        auto *const matches = new abstract_gui::spinner;

        matches->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_MATCHES_REQD);};
        matches->set_value = [=](const int value){filter->set_parameter(filter_anti_tear_c::PARAM_MATCHES_REQD, value);};
        matches->minValue = 1;
        matches->maxValue = 255;

        this->fields.push_back({"Matches req'd", {matches}});
    }

    {
        auto *const tears = new abstract_gui::checkbox;
        tears->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_VISUALIZE_TEARS);};
        tears->set_value = [=](const bool value){filter->set_parameter(filter_anti_tear_c::PARAM_VISUALIZE_TEARS, value);};
        tears->label = "Tears";

        auto *const range = new abstract_gui::checkbox;
        range->get_value = [=]{return filter->parameter(filter_anti_tear_c::PARAM_VISUALIZE_RANGE);};
        range->set_value = [=](const bool value){filter->set_parameter(filter_anti_tear_c::PARAM_VISUALIZE_RANGE, value);};
        range->label = "Range";

        this->fields.push_back({"Visualize", {tears, range}});
    }

    return;
}
