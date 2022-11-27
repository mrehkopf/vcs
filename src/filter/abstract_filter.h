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
 * A frame filter.
 *
 * Applies a pre-set effect (e.g blurring, sharpening, or the like) onto the
 * pixels of a captured frame (or of any raster image).
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

    std::string string_parameter(std::size_t offset) const;

    std::vector<char> parameter_data_block(const std::size_t startIdx = 0, const std::size_t count = ~0) const;

    void set_parameter(const unsigned offset, const double value);

    void set_parameters(const std::vector<std::pair<unsigned, double>> &parameters);

    void set_parameter_string(const unsigned offset, const std::string &string, const std::size_t maxLength = ~0);

    virtual abstract_filter_c* create_clone(void) const = 0;

    virtual std::string name(void) const = 0;

    virtual std::string uuid(void) const = 0;

    virtual filter_category_e category(void) const = 0;

    /*!
     * Applies the filter's image processing to an image's pixels.
     *
     * This function is allowed only to alter the image's pixel values, not e.g. its
     * size.
     */
    virtual void apply(image_s *const image) = 0;

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
    void assert_input_validity(image_s *const image)
    {
        k_assert(
            ((image->resolution.bpp == 32) &&
             (image->resolution.w <= MAX_CAPTURE_WIDTH) &&
             (image->resolution.h <= MAX_CAPTURE_HEIGHT)),
            "Unsupported frame format."
        );

        k_assert(image->pixels, "Null pixel data,");
    }

private:
    std::vector<double> parameterValues;
};

#endif
