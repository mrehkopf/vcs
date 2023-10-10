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
#include "common/assert.h"

struct image_s;

#define CLONABLE_FILTER_TYPE(filter_child_class) \
    abstract_filter_c* create_clone(const std::vector<std::pair<unsigned, double>> &initialParamValues = {}) const override\
    {\
        k_assert((typeid(filter_child_class) == typeid(*this)), "Type mismatch in duplicator.");\
        return new filter_child_class(initialParamValues);\
    }\


#define ASSERT_FILTER_ARGUMENTS(/* image_s* */ image) \
    k_assert( \
        ((image->bitsPerPixel == 32) && \
         (image->resolution.w <= MAX_CAPTURE_WIDTH) && \
         (image->resolution.h <= MAX_CAPTURE_HEIGHT)), \
        "Unsupported image format for applying an image filter." \
    ); \
    if ( \
        (!image->pixels) || \
        (image->resolution.w <= 0) || \
        (image->resolution.h <= 0) \
    ){ \
        return; \
    }

// An image filter. Applies a pre-set effect (e.g blurring, sharpening, or the like)
// onto the pixels of a given image.
class abstract_filter_c
{
public:
    abstract_filter_c(
        const std::vector<std::pair<unsigned /*parameter idx*/, double /*initial value*/>> &parameters = {},
        const std::vector<std::pair<unsigned /*parameter idx*/, double /*initial value*/>> &overrideParamValues = {}
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

    virtual abstract_filter_c* create_clone(const std::vector<std::pair<unsigned, double>> &overrideParams) const = 0;

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

#endif
