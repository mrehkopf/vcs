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

#include <functional>
#include "common/memory/memory_interface.h"
#include "display/display.h"
#include "common/globals.h"
#include "filter/filter_funcs.h"

struct filter_widget_s;

/*!
 * @brief
 * Enumerates the available filter types.
 * 
 * These types correspond in name to the filter functions of the filter
 * functions interface, @ref src/filter/filter_funcs.h.
 *
 * @see
 * filter_category_e
 * 
 * @note
 * Types @a input_gate and @a output_gate are special cases. They aren't filter
 * types as such but rather conditions used by filter chains to decide whether
 * a particular captured frame should be processed by the chain. "Filters" of
 * type @a input_gate specify a resolution in which a given frame must have
 * been captured in order to be processed by the chain, and "filters" of type
 * @a output_gate specify a resolution which must match that currently returned
 * by ks_output_resolution() - the resolution to which VCS will scale the
 * frame - in order for the frame to be processed by the chain. Each frame must
 * satisfy both conditions for the filter chain to act on it.
 */
enum class filter_type_enum_e
{
    blur,
    delta_histogram,
    unique_count,
    unsharp_mask,
    decimate,
    denoise_temporal,
    denoise_nonlocal_means,
    sharpen,
    median,
    crop,
    flip,
    rotate,
    delta_tiles,

    // Special cases. Gates are nodes that in a filter chain only pass or
    // reject images based on their original (input) and scaled (output)
    // resolutions. An image whose input and output resolutions don't match a
    // filter chain's input and output gates' resolution won't be operated on
    // by the filters in the chain.
    input_gate,
    output_gate,
};

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
    filter_c(const std::string &id, const u8 *initialParameterValues = nullptr);
    ~filter_c();

    /*!
     * @brief
     * Provides information about an instance of @ref filter_c.
     */
    struct filter_metadata_s
    {
        /*!
         * The filter instance's user-facing name; e.g. "Blur" for a blur
         * filter.
         * 
         * @note
         * This string may be displayed in the GUI, so it should be
         * sufficiently user-friendly.
         */
        std::string name;

        filter_category_e category;

        filter_type_enum_e type;

        /*!
         * A reference to a relevant filter function provided by
         * the filter functions interface, @ref src/filter/filter_funcs.h,
         * this function applies the filter's image processing to an image's
         * pixels.
         * 
         * This function is expected to operate on the image's pixels in-place.
         * 
         * This function is allowed only to alter the image's pixel values, not
         * e.g. its size.
         */
        std::function<void(FILTER_FUNC_PARAMS)> apply;
    };

    /*!
     * A reference to a corresponding entry in the master list of filter
     * metadata, @ref KNOWN_FILTER_TYPES (@ref src/filter/filter.cpp).
     */
    const filter_metadata_s &metaData;

    /*!
     * A byte array containing the parameter values of this filter instance;
     * e.g. radius for a blur filter.
     * 
     * Assuming VCS is using its default GUI, you can find which parameters
     * are included in this array for each filter type by inspecting
     * @ref src/display/qt/widgets/filter_widgets.cpp.
     */
    heap_bytes_s<u8> parameterData;

    /*!
     * The filter's GUI widget, which provides the end-user with controls
     * for adjusting the filter's parameters.
     */
    filter_widget_s *const guiWidget;

private:
    /*!
     * Creates and returns an instance of a GUI widget for this filter.
     * 
     * The base implementations of filters' GUI widgets can be found in
     * @ref src/display/qt/widgets/filter_widgets.cpp.
     */
    filter_widget_s* create_gui_widget(const u8 *const initialParameterValues = nullptr);
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
void kf_add_filter_chain(std::vector<const filter_c*> newChain);

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
 * Returns the GUI-displayable name string associated with the given filter
 * type.
 * 
 * @see
 * kf_filter_name_for_id(), kf_filter_type_for_id(), kf_filter_id_for_type()
 */
std::string kf_filter_name_for_type(const filter_type_enum_e type);

/*!
 * Returns the GUI-displayable name string associated with the given filter id
 * string.
 * 
 * @see
 * kf_filter_name_for_type(), kf_filter_type_for_id(), kf_filter_id_for_type()
 */
std::string kf_filter_name_for_id(const std::string id);

/*!
 * Returns the id string associated with the given filter type.
 * 
 * @see
 * kf_filter_name_for_type(), kf_filter_name_for_id(), kf_filter_type_for_id()
 */
std::string kf_filter_id_for_type(const filter_type_enum_e type);

/*!
 * Returns the filter type associated with the given filter id string.
 * 
 * @see
 * kf_filter_name_for_type(), kf_filter_name_for_id(), kf_filter_id_for_type()
 */
filter_type_enum_e kf_filter_type_for_id(const std::string id);

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
 *     std::cout << filterType->name << std::endl;
 * }
 * @endcode
 * 
 * This code would print the UUID id string of each of the available filter
 * types.
 * 
 * @code
 * for (auto *filterType: kf_known_filter_types())
 * {
 *     std::cout << kf_filter_id_for_type(filterType->type) << std::endl;
 * }
 * @endcode
 */
std::vector<const filter_c::filter_metadata_s*> kf_known_filter_types(void);

/*!
 * Asks the filter subsystem to create a new instance of @ref filter_c whose
 * @ref filter_type_enum_e type is identified in the master list of filter
 * types by the given UUID id string.
 * 
 * Returns a pointer to the created instance; or @a nullptr on error. The
 * caller can use the pointer - if valid - as an element in a filter chain.
 * 
 * @see
 * kf_add_filter_chain()
 */
const filter_c* kf_create_new_filter_instance(const char *const id);

/*!
 * Asks the filter subsystem to create a new instance of @ref filter_c whose
 * @ref filter_type_enum_e type and initial parameter values are given as
 * arguments.
 * 
 * @p initialParameterValues is an array of bytes that defines the filter's
 * starting parameter values (the end-user can still alter them via the
 * filter's GUI widget). Assuming VCS's default GUI, you can find which
 * parameters are included in this array - and their default values - for each
 * filter type by inspecting @ref src/display/qt/widgets/filter_widgets.cpp. If
 * this argument is @a nullptr, the filter's default parameter values will be
 * used.
 * 
 * Returns a pointer to the created instance; or @a nullptr on error. The
 * caller can use the pointer - if valid - as an element in a filter chain.
 * 
 * @see
 * kf_add_filter_chain()
 */
const filter_c* kf_create_new_filter_instance(const filter_type_enum_e type, const u8 *const initialParameterValues = nullptr);

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
