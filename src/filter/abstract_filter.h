/*
 * 2018, 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ABSTRACT_FILTER_H
#define VCS_FILTER_ABSTRACT_FILTER_H

#include <vector>
#include <string>
#include "filter/filter.h"

#define CLONABLE_FILTER_TYPE(filter_child_class) \
    abstract_filter_c* create_clone(void) const override\
    {\
        k_assert((typeid(filter_child_class) == typeid(*this)), "Type mismatch in duplicator.");\
        return new filter_child_class(this->parameters());\
    }\

/*!
 * @brief
 * An image filter.
 *
 * Applies a pre-set effect (e.g blurring, sharpening, or the like) onto the
 * pixels of an image.
 */
class abstract_filter_c
{
public:
    abstract_filter_c(const std::vector<std::pair<unsigned /*parameter idx*/, double /*initial value*/>> &parameters = {},
                      const std::vector<std::pair<unsigned /*parameter idx*/, double /*override value*/>> &overrideParameterValues = {});

    virtual ~abstract_filter_c(void);

    const std::vector<filtergui_field_s>& gui_description(void) const;

    unsigned num_parameters(void) const;

    double parameter(const unsigned offset) const;

    std::vector<std::pair<unsigned, double>> parameters(void) const;

    void set_parameter(const unsigned offset, const double value);

    void set_parameters(const std::vector<std::pair<unsigned, double>> &parameters);

    virtual abstract_filter_c* create_clone(void) const = 0;

    virtual std::string name(void) const = 0;

    virtual std::string uuid(void) const = 0;

    virtual filter_category_e category(void) const = 0;

    /*!
     * Applies the filter's image processing to an image's pixels.
     *
     * This function is allowed only to alter the image's pixel values, not
     * e.g. its size.
     */
    virtual void apply(u8 *const pixels, const resolution_s &r) = 0;

    /*!
     * The filter's GUI widget, which provides the end-user with controls for
     * adjusting the filter's parameters.
     *
     * The widget is provided as a framework-independent description of the
     * components that the widget should have. It's then up to VCS's chosen GUI
     * framework to interpret the instructions to create the actual widget.
     */
    filtergui_c *guiDescription = nullptr;

protected:
    /*!
     * Triggers an assertion failure if @p pixels or @p r are out of bounds for
     * a filter's apply() function.
     */
    void assert_input_validity(const u8 *const pixels, const resolution_s &r)
    {
        k_assert(((r.bpp == 32) &&
                  (r.w <= MAX_CAPTURE_WIDTH) &&
                  (r.h <= MAX_CAPTURE_HEIGHT)),
                 "Unsupported frame format.");

        k_assert(pixels, "Null pixel data,");
    }

private:
    std::vector<double> parameterValues;
};

#endif
