/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CROP_FILTER_CROP_H
#define VCS_FILTER_FILTERS_CROP_FILTER_CROP_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"
#include "scaler/scaler.h"

class filter_crop_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_crop_c)

    enum { PARAM_X,
           PARAM_Y,
           PARAM_WIDTH,
           PARAM_HEIGHT,
           PARAM_SCALER };

    enum { SCALE_LINEAR,
           SCALE_NEAREST,
           SCALE_NONE };

    filter_crop_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_X, 0},
            {PARAM_Y, 0},
            {PARAM_WIDTH, 640},
            {PARAM_HEIGHT, 480},
            {PARAM_SCALER, SCALE_NONE}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *xPos = filtergui::spinner(filter, PARAM_X);
            xPos->maxValue = MAX_CAPTURE_WIDTH;
            xPos->minValue = 0;
            auto *yPos = filtergui::spinner(filter, PARAM_Y);
            yPos->maxValue = MAX_CAPTURE_HEIGHT;
            yPos->minValue = 0;
            gui->fields.push_back({"XY", {xPos, yPos}});

            auto *width = filtergui::spinner(filter, PARAM_WIDTH);
            width->maxValue = MAX_CAPTURE_WIDTH;
            width->minValue = 1;
            auto *height = filtergui::spinner(filter, PARAM_HEIGHT);
            height->maxValue = MAX_CAPTURE_HEIGHT;
            height->minValue = 1;
            gui->fields.push_back({"Size", {width, height}});

            auto *scaler = filtergui::combo_box(filter, PARAM_SCALER);
            scaler->items = {"Linear", "Nearest", "None"};
            gui->fields.push_back({"Scaler", {scaler}});
        });
    }

    void apply(image_s *const image) override
    {
        const unsigned x = this->parameter(PARAM_X);
        const unsigned y = this->parameter(PARAM_Y);
        const unsigned w = this->parameter(PARAM_WIDTH);
        const unsigned h = this->parameter(PARAM_HEIGHT);
        const unsigned scalerType = this->parameter(PARAM_SCALER);

        const int cvScaler = ([scalerType]
        {
            switch (scalerType)
            {
                case SCALE_LINEAR: return cv::INTER_LINEAR;
                case SCALE_NEAREST: return cv::INTER_NEAREST;
                case SCALE_NONE: return cv::INTER_NEAREST;
                default: k_assert(0, "Unknown scaler type for the crop filter."); return cv::INTER_NEAREST;
            }
        })();

        if ((x + w) > image->resolution.w)
        {
            KS_PRINT_FILTER_ERROR("The crop region is wider than the input image");
        }
        else if ((y + h) > image->resolution.h)
        {
            KS_PRINT_FILTER_ERROR("The crop region is taller than the input image");
        }
        else
        {
            cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
            cv::Mat cropped = output(cv::Rect(x, y, w, h)).clone();

            // If the user doesn't want scaling, just append some black borders around the
            // cropping. Otherwise, stretch the cropped region to fill the entire frame.
            if (scalerType == SCALE_NONE) cv::copyMakeBorder(cropped, output, y, (image->resolution.h - (h + y)), x, (image->resolution.w - (w + x)), cv::BORDER_CONSTANT, 0);
            else cv::resize(cropped, output, output.size(), 0, 0, cvScaler);
        }

        return;
    }

    std::string uuid(void) const override { return "2448cf4a-112d-4d70-9fc1-b3e9176b6684"; }
    std::string name(void) const override { return "Crop"; }
    filter_category_e category(void) const override { return filter_category_e::distort; }

private:
};

#endif
