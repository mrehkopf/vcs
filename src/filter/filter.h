/*
 * 2018, 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*! @file
 *
 * @brief
 * The filter interface provides image manipulation of captured frames.
 *
 * VCS's filter subsystem enables the end-user to apply image filters to
 * captured frames; this allows for effects like blurring, rotating, and
 * sharpening. Filters only operate on an image's pixel values, however, and
 * aren't allowed to resize the image. The scaler subsystem,
 * @ref src/scaler/scaler.h, is responsible for image scaling.
 *
 * The filter interface, the interface to the filter subsystem, provides both
 * the @ref filter_c class that encapsulates filter functionality, and
 * functions to control which filters the subsystem should apply to which
 * captured frames.
 *
 * The @ref filter_c class is the basic building block of VCS's pixel
 * manipulation capabilities. Each instance of @ref filter_c holds
 * end-user-customizable parameters (like radius for a blur filter) and a GUI
 * widget with which the user can customize those parameters, as well as
 * providing a function with which the filter can be applied to an image's
 * pixels.
 *
 * Instances of @ref filter_c are organized into filter chains, which are
 * end-user-defined collections of one or more filters that together are to be
 * applied to captured frames' pixels. Each filter chain begins with an input
 * condition, followed by one or more filter instances, followed by an output
 * condition. The input and output conditions select which frames the chain's
 * filters are applied to: the input condition defines a capture resolution and
 * the output condition a scaled resolution, such that frames which were
 * captured at the input condition's resolution and which are to be scaled by
 * VCS to  a resolution matching the the output conditions's will have the
 * chain's filters applied to them.
 *
 * For each captured frame, before it's been scaled by the scaler subsystem
 * (@ref src/scaler/scaler.h), VCS will pass the frame's pixel data to
 * @ref kf_apply_filter_chain() of the filter subsystem. This function is
 * responsible for selecting a matching filter chain - if any - and applying
 * its filters to the frame's pixels.
 *
 * The master list of available filter types, @ref KNOWN_FILTER_TYPES, is
 * defined in @ref src/filter/filter.cpp, and can also be queried via the
 * filter interface with @ref kf_known_filter_types(). In the master list, each
 * filter type is associated with a permanent UUID id string by which the type
 * can be referenced even when its display name or other properties might
 * change.
 *
 * General usage:
 *
 * 1. Initialize the filter subsystem with kf_initialize_filters().
 *
 * 2. Create one or more filters with kf_create_new_filter_instance().
 *
 * 3. Create filter chains out of the filter instances and pass the chains to
 *    kf_add_filter_chain().
 *
 * 4. Pass captured frames to kf_apply_filter_chain() to have the relevant
 *    filter chain's filters be applied to the frames' pixels.
 *
 * 5. Call kf_release_filters() on program exit to free allocated memory.
 */

#ifndef VCS_FILTER_FILTER_H
#define VCS_FILTER_FILTER_H

#include <unordered_map>
#include <functional>
#include <cstring>
#include "common/memory/memory_interface.h"
#include "filter/filtergui.h"
#include "display/display.h"
#include "common/globals.h"

// The parameters to filters' apply() function.
#define FILTER_APPLY_FUNCTION_PARAMS u8 *const pixels /*32-bit BGRA*/, const resolution_s &r

// The parameters to filters' constructor.
#define FILTER_CTOR_FUNCTION_PARAMS const std::vector<std::pair<unsigned, double>> &initialParameterValues = {}

#define CLONABLE_FILTER_TYPE(filter_child_class) \
    filter_c* create_clone(void) const override\
    {\
        k_assert((typeid(filter_child_class) == typeid(*this)), "Type mismatch in duplicator.");\
        return new filter_child_class(this->parameters());\
    }\

// Call this macro in filters' apply() functions to veryfy that the function
// arguments are valid.
#define VALIDATE_FILTER_INPUT \
{\
    k_assert(((r.bpp == 32) &&\
              (r.w <= MAX_CAPTURE_WIDTH) &&\
              (r.h <= MAX_CAPTURE_HEIGHT)),\
             "Unsupported frame format.");\
    if (!pixels) return;\
}

/*!
 * @brief
 * Enumerates the categories of filter types.
 *
 * These categories exist e.g. for the benefit of the GUI, allowing it to
 * more intuitively order the filter types in menus.
 *
 * @see
 * filter_type_enum_e
 */
enum class filter_category_e
{
    // Filters that reduce the frame's fidelity; e.g. blur, decimate.
    reduce,

    // Filters that enhance the frame's fidelity; e.g. sharpen, denoise.
    enhance,

    // Filters that modify the frame's geometry; e.g. rotation, cropping.
    distort,

    // Filters that provide information about the frame; e.g. noise level,
    // frame rate estimate.
    meta,

    gate_input,
    gate_output,
};

/*!
 * @brief
 * An image filter.
 *
 * Applies a pre-set effect (e.g blurring, sharpening, or the like) onto the
 * pixels of an image.
 */
class filter_c
{
public:
    filter_c(const std::vector<std::pair<unsigned /*parameter idx*/, double /*initial value*/>> &parameters = {},
             const std::vector<std::pair<unsigned /*parameter idx*/, double /*override value*/>> &overrideParameterValues = {});

    virtual ~filter_c(void);

    const std::vector<filtergui_field_s>& gui_description(void) const;

    unsigned num_parameters(void) const;

    double parameter(const unsigned offset) const;

    std::vector<std::pair<unsigned, double>> parameters(void) const;

    void set_parameter(const unsigned offset, const double value);

    void set_parameters(const std::vector<std::pair<unsigned, double>> &parameters);

    virtual filter_c* create_clone(void) const = 0;

    virtual std::string name(void) const = 0;

    virtual std::string uuid(void) const = 0;

    virtual filter_category_e category(void) const = 0;

    /*!
     * Applies the filter's image processing to an image's pixels.
     *
     * This function is allowed only to alter the image's pixel values, not
     * e.g. its size.
     */
    virtual void apply(FILTER_APPLY_FUNCTION_PARAMS) = 0;

    /*!
     * The filter's GUI widget, which provides the end-user with controls for
     * adjusting the filter's parameters.
     *
     * The widget is provided as a framework-independent description of the
     * components that the widget should have. It's then up to VCS's chosen GUI
     * framework to interpret the instructions to create the actual widget.
     */
    filtergui_c *guiDescription = nullptr;

private:
    std::vector<double> parameterValues;
};

/*!
 * Asks VCS to initialize the filter subsystem.
 *
 * This function should be called only once, on program launch.
 */
void kf_initialize_filters(void);

/*!
 * Asks VCS to release the filter subsystem.
 *
 * This function should be called only once, on program termination.
 */
void kf_release_filters(void);

/*!
 * Asks VCS to add the given filter chain to its list of known filter chains.
 *
 * The filter elements in the chain must be pointers to instances of
 * @ref filter_c created via kf_create_new_filter_instance().
 *
 * Each chain must begin with an input condition (filter_c of type
 * @ref filter_type_enum_e @a input_gate) and end with an output condition
 * (filter_c of type @ref filter_type_enum_e @a output_gate).
 *
 * @see
 * kf_remove_all_filter_chains(),
 * kf_create_new_filter_instance(const char *const),
 * kf_create_new_filter_instance(const filter_type_enum_e , const u8 *const)
 */
void kf_add_filter_chain(std::vector<filter_c*> newChain);

/*!
 * Asks the filter subsystem to clear its list of known filter chains. This
 * will effectively disable filtering until new filter chains are added.
 *
 * Once cleared, the list of filter chains can only be restored by (re-)adding
 * the desired chains via kf_add_filter_chain().
 *
 * @see
 * kf_set_filtering_enabled(), kf_add_filter_chain()
 */
void kf_remove_all_filter_chains(void);

/*!
 * Applies to the @p pixels of an image whose resolution is @p r the first
 * known filter chain whose input condition matches @p r and whose output
 * condition matches VCS's current output resolution as given from
 * ks_output_resolution().
 *
 * If no matching filter chain is found, no filter will be applied.
 *
 * @see
 * kf_add_filter_chain()
 */
void kf_apply_filter_chain(u8 *const pixels, const resolution_s &r);

/*!
 * Returns a list of the filter types that're available in the filter
 * subsystem.
 *
 * The following sample code would print the names of all of the available
 * filter types:
 *
 * @code
 * for (auto *filterType: kf_known_filter_types())
 * {
 *     std::cout << filterType->name() << std::endl;
 * }
 * @endcode
 */
const std::vector<const filter_c *>& kf_known_filter_types(void);

/*!
 * Asks the filter subsystem to create a new instance of @ref filter_c whose
 * @ref filter_type_enum_e type is identified by the given type and with the
 * given initial parameter values.
 *
 * Returns a pointer to the created instance; or @a nullptr on error. The
 * caller can use the pointer - if valid - as an element in a filter chain.
 *
 * @see
 * kf_add_filter_chain()
 */
filter_c* kf_create_new_filter_instance(const std::string &filterTypeUuid,
                                        const std::vector<std::pair<unsigned, double>> &initialParameterValues = {});

/*!
 * Asks the filter subsystem to delete the given instance of @ref filter_c.
 *
 * Once deleted, this instance can no longer be used in filter chains.
 *
 * @note
 * If you want to delete all filter instances on program exit, you can instead
 * call kf_release_filters(). This is VCS's default exit behavior.
 *
 * @see
 * kf_create_new_filter_instance(const char *const),
 * kf_create_new_filter_instance(const filter_type_enum_e , const u8 *const)
 */
void kf_delete_filter_instance(const filter_c *const filter);

/*!
 * Returns true if the given filter UUID corresponds to a known filter; false
 * otherwise.
 */
bool kf_is_known_filter_type(const std::string &filterTypeUuid);

/*!
 * Toggle the filter subsystem on/off.
 *
 * While the filter subsystem is disabled, no filters will be applied to
 * captured frames.
 *
 * @see
 * kf_is_filtering_enabled(), kf_remove_all_filter_chains()
 */
void kf_set_filtering_enabled(const bool enabled);

/*!
 * Returns true if the filter subsystem is currently enabled; false otherwise.
 *
 * While the filter subsystem is disabled, no filters will be applied to
 * captured frames.
 *
 * @see
 * kf_set_filtering_enabled()
 */
bool kf_is_filtering_enabled(void);

/*!
 * @warning
 * This function is scheduled for removal and should not be used.
 */
std::vector<int> kf_find_capture_alignment(u8 *const pixels, const resolution_s &r);

#endif
