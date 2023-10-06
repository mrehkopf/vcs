/*
 * 2018, 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*
 * The filter subsystem interface.
 * 
 * The filter subsystem provides facilities for manipulating captured frames'
 * pixel data.
 * 
 * The building block of this subsystem is the image filter. They're subclasses of
 * abstract_filter_c that modify image pixels in some way: blurring, sharpening,
 * rotation, and so on.
 * 
 * Instances of image filters are organized into filter chains. A filter chain
 * consists of one or more filters, each of which will be applied in turn to an
 * input frame. A filter chain consisting of a blur filter, a sharpen filter, and
 * a rotation filter would first blur the frame, then sharpen it, and finally
 * rotate it.
 * 
 * Each filter chain is associated with an input condition and an output condition,
 * based on which the subsystem makes a choice about which of the chains to apply
 * to an input frame. A chain's input condition dictates the resolution the input
 * frame must be in to be acted on by the chain's filters, and the output condition
 * specifies the resolution the scaler subsystem must be outputting in for the
 * chain to be applied. The first filter chain whose input and output conditions
 * are fulfilled will be applied to the input frame.
 * 
 * For example, a filter chain with an input condition of 640 x 480 and an output
 * condition of 800 x 600 will be applied to an input frame if the frame's resolution
 * is 640 x 480 and the scaler subsystem is set to produce its output in 800 x 600
 * (i.e. it'll scale this frame to 800 x 600 at a later stage in VCS's capture
 * pipeline). If there's a second filter chain with these same conditions, only
 * the first chain will be applied and not the second.
 *
 * ## Usage
 *
 *   1. Call kf_initialize_filters() to initialize the filter subsystem. Note that
 *      this function should only be called once per program execution.
 *
 *   2. Use kf_create_filter_instance() to create instances of filters:
 *      @code
 *      // Create an instance of a blurring filter.
 *      auto *blur = kf_create_filter_instance<filter_blur_c>();
 * 
 *      // You can also use the non-templated instancer, which takes a UUID string
 *      // identifying the type of filter to create.
 *      auto *flip = kf_create_filter_instance("80a3ac29-fcec-4ae0-ad9e-bbd8667cc680");
 * 
 *      // "80a3ac29-fcec-4ae0-ad9e-bbd8667cc680" == filter_flip_c().uuid().
 *      @endcode
 * 
 *   3. Get and set filters' parameter values:
 *      @code
 *      auto *blur = kf_create_filter_instance<filter_blur_c>();
 * 
 *      // Increase the blurring radius.
 *      const double radius = blur->parameter(filter_blur_c::PARAM_KERNEL_SIZE);
 *      blur->set_parameter(filter_blur_c::PARAM_KERNEL_SIZE, (radius + 1));
 * 
 *      // Use a box kernel instead of the default Gaussian.
 *      blur->set_parameter(filter_blur_c::PARAM_TYPE, filter_blur_c::BLUR_BOX);
 * 
 *      // Note: Each filter also includes a GUI widget that provides the end-user graphical
 *      // controls for adjusting the filter parameters. VCS's display subsystem provides an
 *      // interactive filter graph that exposes these widgets to the user.
 *      @endcode
 *
 *   4. Combine filters into filter chains:
 *      @code
 *      // Create some filters.
 *      auto *blur = kf_create_filter_instance<filter_blur_c>();
 *      auto *flip = kf_create_filter_instance<filter_flip_c>();
 * 
 *      // Create the chain's input and output conditions. This chain will be applied to
 *      // frames of size 640 x 480 when the scaler subsystem's output size is 800 x 600.
 *      auto *in = kf_create_filter_instance<filter_input_gate_c>({{0, 640}, {1, 480}});
 *      auto *out = kf_create_filter_instance<filter_output_gate_c>({{0, 800}, {1, 600}});
 * 
 *      // Create the chain. Note that chains must begin with an input condition and end
 *      // with an output condition.
 *      std::vector<abstract_filter_c*> chain = {in, blur, flip, out};
 * 
 *      // Make the filter subsystem aware of the chain.
 *      kf_register_filter_chain(chain);
 * 
 *      // Note: VCS's display subsystem provides the end-user an interactive filter
 *      // graph for building filter chains via the GUI.
 *      @endcode
 * 
 *   5. Apply suitable filter chains to captured frames:
 *      @code
 *      ev_new_captured_frame.listen([](const captured_frame_s &frame)
 *      {
 *          image_s image(frame.pixels.data, frame.r);
 *          kf_apply_matching_filter_chain(&image);
 *      });
 *      @endcode
 * 
 *   6. (Optional) Apply filters individually:
 *      @code
 *      auto *blur = kf_create_filter_instance<filter_blur_c>();
 * 
 *      // Filter a dummy image of 1 x 2 resolution (BGRA/8888 format).
 *      uint8_t pixels[8];
 *      resolution_s resolution = {1, 2, 32};
 *      blur->apply(pixels, resolution);
 * 
 *      // When we no longer need the filter.
 *      kf_delete_filter_instance(blur);
 *      @endcode
 * 
 *   7. VCS will automatically release the subsystem on program termination.
 *
 */

#ifndef VCS_FILTER_FILTER_H
#define VCS_FILTER_FILTER_H

#include <unordered_map>
#include <functional>
#include <cstring>
#include "common/abstract_gui.h"
#include "main.h"

class abstract_filter_c;
struct resolution_s;
struct image_s;

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

// Initializes the filter subsystem, allocating its memory buffers etc.
subsystem_releaser_t kf_initialize_filters(void);

// Adds the given filter chain to the filter subsystem's list of known chains.
// This makes the chain available for use by kf_apply_matching_filter_chain().
//
// The chain's filter instances must be created using kf_create_filter_instance().
//
// A filter chain must begin with a filter of type filter_category_e::input_condition
// and end with a filter of type filter_category_e::output_condition|output_scaler.
void kf_register_filter_chain(std::vector<abstract_filter_c*> newChain);

// Clears the filter subsystem's list of registered filter chains.
//
// The chains' filter instances won't be deallocated. You need to call
// kf_delete_filter_instance() if you want to deallocate them.
void kf_unregister_all_filter_chains(void);

// Applies to the pixels of an image the first filter chain registered with
// kf_register_filter_chain() whose input condition matches the image's resolution
// and whose output condition matches the scaler subsystem's current output
// resolution. If there's no chain that matches these conditions, no chain will
// be applied.
//
// If the filter subsystem is disabled, or if there are no registered filter
// chains, calling this function has no effect.
//
// Returns a pointer to the filter chain's output scaler, if the chain has one;
// otherwise, returns nullptr.
abstract_filter_c* kf_apply_matching_filter_chain(image_s *const dstImage);

const std::vector<const abstract_filter_c*>& kf_available_filter_types(void);

// Creates a new instance of a filter, whose type is identified with a UUID and
// whose initial parameters values are given.
//
// Returns a pointer to the created instance, or nullptr on error. The caller
// can use the pointers obtained from this function to create filter chains.
//
// The returned pointer's memory is managed by the filter subsystem; the caller
// shouldn't deallocate it. Its release can be requested via
// kf_delete_filter_instance().
abstract_filter_c* kf_create_filter_instance(const std::string &filterTypeUuid, const filter_params_t &initialParamValues = {});

template <class T>
abstract_filter_c* kf_create_filter_instance(const filter_params_t &initialParamValues = {})
{
    return kf_create_filter_instance(T().uuid(), initialParamValues);
}

// Deallocates a filter instance created by kf_create_filter_instance().
//
// The caller must first unregister any filter chains that're using this filter.
void kf_delete_filter_instance(const abstract_filter_c *const filter);

// Returns true if the given filter type UUID identifies a filter type available
// via this subsystem interface; false otherwise.
bool kf_is_known_filter_uuid(const std::string &filterTypeUuid);

// Enable or disable the filter subsystem.
//
// This state has an effect on kf_apply_matching_filter_chain() such that the function
// will do nothing if the filter subsystem is disabled. Other functionality of
// the interface is not affected.
void kf_set_filtering_enabled(const bool enabled);

bool kf_is_filtering_enabled(void);

#endif
