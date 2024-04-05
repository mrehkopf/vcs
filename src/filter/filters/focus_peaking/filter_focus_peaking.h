/*
 * 2024 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FILTER_FOCUS_PEAKING_H
#define VCS_FILTER_FILTERS_FILTER_FOCUS_PEAKING_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_focus_peaking_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_focus_peaking_c)

    enum { PARAM_RED,
           PARAM_GREEN,
           PARAM_BLUE,
           PARAM_THRESHOLD };

    filter_focus_peaking_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_RED, 255},
            {PARAM_GREEN, 0},
            {PARAM_BLUE, 255},
            {PARAM_THRESHOLD, 75}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *red = filtergui::spinner(filter, PARAM_RED);
            red->minValue = 0;
            red->maxValue = 255;

            auto *green = filtergui::spinner(filter, PARAM_GREEN);
            green->minValue = 0;
            green->maxValue = 255;

            auto *blue = filtergui::spinner(filter, PARAM_BLUE);
            blue->minValue = 0;
            blue->maxValue = 255;

            auto *threshold = filtergui::spinner(filter, PARAM_THRESHOLD);
            threshold->on_change = [filter](int value){filter->set_parameter(PARAM_THRESHOLD, value);};
            threshold->minValue = 0;
            threshold->maxValue = 255;

            gui->fields.push_back({"Color", {red, green, blue}});
            gui->fields.push_back({"Threshold", {threshold}});
        });
    }

    void apply(image_s *const image) override
    {
        static uint8_t *const scratch = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

        const unsigned threshold =  this->parameter(PARAM_THRESHOLD);
        const cv::Scalar overlayColor(
            this->parameter(PARAM_BLUE),
            this->parameter(PARAM_GREEN),
            this->parameter(PARAM_RED)
        );

        cv::Mat src = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
        cv::Mat edges = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, scratch);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);

        cv::cvtColor(src, edges, cv::COLOR_BGRA2GRAY);
        cv::Laplacian(edges, edges, CV_8U);
        output.setTo(overlayColor, (edges > threshold));
    }

    std::string name(void) const override { return "Focus peaking"; }
    std::string uuid(void) const override { return "0ce717f1-aa46-449c-97da-336fc4de701c"; }
    filter_category_e category(void) const override { return filter_category_e::meta; }

private:
};

#endif
