/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_CRT_CRT_H
#define VCS_FILTER_FILTERS_CRT_CRT_H

#include "filter/abstract_filter.h"

class filter_crt_c : public abstract_filter_c
{
public:
    CLONABLE_FILTER_TYPE(filter_crt_c)

    enum { PARAM_CRT_TYPE };

    filter_crt_c(const filter_params_t &initialParamValues = {}) :
        abstract_filter_c({
            {PARAM_CRT_TYPE, 0}
        }, initialParamValues)
    {
        this->gui = new abstract_gui_s([this](abstract_gui_s *const gui)
        {
            auto *const blurType = new abstract_gui::combo_box;
            blurType->get_value = [this]{return this->parameter(PARAM_CRT_TYPE);};
            blurType->set_value = [this](int value){this->set_parameter(PARAM_CRT_TYPE, std::max(0, value));};
            blurType->items = {"Crap"};
            gui->fields.push_back({"Screen quality", {blurType}});
        });
    }

    void apply(image_s *const image) override;
    std::string uuid(void) const override { return "61a2eaed-a177-42fe-bf9e-5302e4907b51"; }
    std::string name(void) const override { return "CRT"; }
    filter_category_e category(void) const override { return filter_category_e::reduce; }

private:
};

#endif
