/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ABSTRACT_FILTER_H
#define VCS_FILTER_ABSTRACT_FILTER_H

#include <vector>
#include <string>
#include "common/abstract_gui.h"
#include "common/assert.h"
#include "display/display.h"

#define CLONABLE_FILTER_TYPE(filter_child_class) \
    abstract_filter_c* create_clone(const filter_params_t &initialParamValues = {}) const override\
    {\
        k_assert((typeid(filter_child_class) == typeid(*this)), "Type mismatch in filter cloning.");\
        return new filter_child_class(initialParamValues);\
    }\

typedef std::vector<std::pair<unsigned /*param idx*/, double /*param value*/>> filter_params_t;

// Enumerates the functional categories into which filters can be divided.
//
// These categories exist e.g. for the benefit of the VCS GUI, allowing a more
// structured listing of filters in menus.
enum class filter_category_e
{
    // Filters that reduce the image's fidelity. For example: blur, decimate.
    reduce,

    // Filters that enhance the image's fidelity. For example: sharpen, denoise.
    enhance,

    // Filters that modify the image's geometry. For example: rotate, crop.
    distort,

    // Filters that provide information about the image. For example: frame rate
    // estimate, noise histogram.
    meta,

    // Filters that resize the final image. Note that these filters can operate
    // only as the last link in the filter chain, rather than as intermediate
    //scalers.
    output_scaler,

    // Special case, not for use by filters. Used as a control in filter chains.
    input_condition,

    // Special case, not for use by filters. Used as a control in filter chains.
    output_condition,
};

// An image filter. Applies a pre-set effect (e.g blurring, sharpening, or the like)
// onto the pixels of a given image.
class abstract_filter_c
{
public:
    abstract_filter_c(
        const filter_params_t &parameters = {},
        const filter_params_t &overrideParamValues = {}
    );

    virtual ~abstract_filter_c(void);

    unsigned num_parameters(void) const;

    double parameter(const unsigned offset) const;

    std::vector<std::pair<unsigned, double>> parameters(void) const;

    std::string string_parameter(std::size_t offset) const;

    std::vector<char> parameter_data_block(const std::size_t startIdx = 0, const std::size_t count = ~0) const;

    void set_parameter(const unsigned offset, const double value);

    void set_parameters(const filter_params_t &parameters);

    void set_parameter_string(const unsigned offset, const std::string &string, const std::size_t maxLength = ~0);

    virtual abstract_filter_c* create_clone(const filter_params_t &overrideParams) const = 0;

    virtual std::string name(void) const = 0;

    virtual std::string uuid(void) const = 0;

    virtual filter_category_e category(void) const = 0;

    // Applies the filter's effect on the input image.
    virtual void apply(image_s *const image) = 0;

    // The filter's GUI widget, which appears in VCS's filter graph and provides
    // the user with controls for adjusting the filter's parameters.
    abstract_gui_s *gui = nullptr;

private:
    std::vector<double> parameterValues;
};

// Shorthands for creating and initializing widgets for filter GUIs.
namespace filtergui
{
    const auto text_edit = [](abstract_filter_c *const filter, const unsigned paramId)
    {
        auto *const widget = new abstract_gui::text_edit;
        widget->on_change = [filter, paramId](const std::string &text){filter->set_parameter_string(paramId, text);};
        widget->text = filter->string_parameter(paramId);
        return widget;
    };

    const auto spinner = [](abstract_filter_c *const filter, const unsigned paramId)
    {
        auto *widget = new abstract_gui::spinner;
        widget->on_change = [filter, paramId](const int value){filter->set_parameter(paramId, value);};
        widget->initialValue = filter->parameter(paramId);
        return widget;
    };

    const auto combo_box = [](abstract_filter_c *const filter, const unsigned paramId)
    {
        auto *widget = new abstract_gui::combo_box;
        widget->on_change = [filter, paramId](const int index){filter->set_parameter(paramId, index);};
        widget->initialIndex = filter->parameter(paramId);
        return widget;
    };

    const auto double_spinner = [](abstract_filter_c *const filter, const unsigned paramId)
    {
        auto *widget = new abstract_gui::double_spinner;
        widget->on_change = [filter, paramId](const double value){filter->set_parameter(paramId, value);};
        widget->initialValue = filter->parameter(paramId);
        return widget;
    };

    const auto checkbox = [](abstract_filter_c *const filter, const unsigned paramId)
    {
        auto *widget = new abstract_gui::checkbox;
        widget->on_change = [filter, paramId](const bool isChecked){filter->set_parameter(paramId, isChecked);};
        widget->initialState = filter->parameter(paramId);
        return widget;
    };
}

#endif
