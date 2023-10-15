/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FILTER_BLUR_H
#define VCS_FILTER_FILTERS_FILTER_BLUR_H

#include <opencv2/imgproc/imgproc.hpp>
#include "filter/abstract_filter.h"

class filter_blur_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_blur_c)

    enum { PARAM_TYPE,
           PARAM_KERNEL_SIZE };

    enum { BLUR_BOX = 0,
           BLUR_GAUSSIAN = 1 };
    
    filter_blur_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_KERNEL_SIZE, 1},
            {PARAM_TYPE, BLUR_GAUSSIAN}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([filter = this](abstract_gui_s *const gui)
        {
            auto *blurType = filtergui::combo_box(filter, PARAM_TYPE);
            blurType->items = {"Box", "Gaussian"};
            gui->fields.push_back({"Type", {blurType}});

            auto *blurRadius = filtergui::double_spinner(filter, PARAM_KERNEL_SIZE);
            blurRadius->minValue = 0.1;
            blurRadius->maxValue = 99;
            blurRadius->numDecimals = 2;
            gui->fields.push_back({"Radius", {blurRadius}});
        });
    }

    void apply(image_s *const image) override
    {
        const double kernelSize = this->parameter(PARAM_KERNEL_SIZE);
        cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);

        if (this->parameter(PARAM_TYPE) == BLUR_GAUSSIAN)
        {
            cv::GaussianBlur(output, output, cv::Size(0, 0), kernelSize);
        }
        else
        {
            const uint8_t kernelW = ((int(kernelSize) * 2) + 1);
            cv::blur(output, output, cv::Size(kernelW, kernelW));
        }
    }

    std::string name(void) const override { return "Blur"; }
    std::string uuid(void) const override { return "a5426f2e-b060-48a9-adf8-1646a2d3bd41"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
