/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*
 * The scaler subsystem interface.
 * 
 * The scaler subsystem provides facilities to VCS for scaling captured frames.
 * 
 * When an input frame is scaled, its pixel data is copied into the subsystem's
 * frame buffer, from which callers can fetch it until the next image is scaled
 * (or until the buffer's pixel data is modified in some other way).
 * 
 * By default, it's the state of the scaler subsystem's frame buffer that gets
 * displayed to the end-user in VCS's capture window.
 * 
 * ## Usage
 * 
 *   1. Call ks_initialize_scaler() to initialize the subsystem. Note that this
 *      function should be called only once per program execution.
 * 
 *   2. Use setter functions to customize scaler options:
 *      @code
 *      ks_set_scaling_multiplier(0.75);
 *      ks_set_downscaling_filter("Linear");
 *      ks_set_base_resolution({640, 480});
 *      @endcode
 * 
 *   3. Feed captured frames into ks_scale_frame(), then read the scaled output from
 *      ks_frame_buffer():
 *      @code
 *      ks_scale_frame(frame);
 *      const auto &scaledImage = ks_frame_buffer();
 *      @endcode
 * 
 *   4. You can automate the scaling of captured frames using event listeners:
 *      @code
 *      // Executed each time the capture subsystem reports a new captured frame.
 *      ev_new_captured_frame.listen([](const captured_frame_s &frame)
 *      {
 *          ks_scale_frame(frame);
 *      });
 * 
 *      // Executed each time the scaler subsystem produces a new scaled image.
 *      ev_new_output_image.listen([](const image_s &image)
 *      {
 *          printf("A frame was scaled to %lu x %lu.\n", image.resolution.w, image.resolution.h);
 *      });
 *      @endcode
 *
 *   5. VCS will automatically release the scaler subsystem on program exit.
 * 
 */

#ifndef VCS_SCALER_SCALER_H
#define VCS_SCALER_SCALER_H

#include "common/globals.h"
#include "common/vcs_event/vcs_event.h"
#include "filter/filters/render_text/font_5x3.h"
#include "main.h"

class abstract_filter_c;
struct captured_frame_s;
struct image_s;

// Callable inside a filter's apply() function to inform the user of an error
// related to the filter's operation. The error string will be printed onto the
// current output frame.
#define KS_PRINT_FILTER_ERROR(errorString) \
    font_5x3_c().render(("FILTER ERROR [" + (this->name()) + "]:\n" + errorString), image, 0, 0, 2, {0, 0, 255}, {0, 0, 0});

struct image_scaler_s
{
    // The scaler's display name. Will be shown in the GUI etc.
    std::string name;

    // A function that executes the scaler on the given pixels.
    void (*apply)(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding);
};

// Returns the resolution to which the scaler will scale input frames prior to
// the application of resolution-influencing modifiers like a scaling multiplier.
//
// In most cases, you'd be interested in ks_output_resolution(), instead.
resolution_s ks_base_resolution(void);

// Returns the resolution to which the scaler would currently scale an input frame.
resolution_s ks_output_resolution(void);

bool ks_is_custom_scaler_active(void);

subsystem_releaser_t ks_initialize_scaler(void);

// Applies scaling to the given frame's pixels and stores the result in the scaler
// subsystem's frame buffer. The input data are not modified.
//
// After this call, the scaled image is available via ks_frame_buffer().
void ks_scale_frame(const captured_frame_s &frame);

void ks_set_scaling_multiplier(void);

void ks_set_scaling_multiplier_enabled(const bool enabled);

// Sets the resolution to which input frames are to be scaled, before applying
// size modifiers like scaling multiplier. By default, the scaler will apply the
// modifiers to each input frame's own resolution.
void ks_set_base_resolution(const resolution_s &r);

// Enables or disables the scaler subsystem's overridable base resolution.
//
// If disabled, the base resolution will be the resolution of the input frame.
void ks_set_base_resolution_enabled(const bool enabled);

// Draws the given status message into the scaler subsystem's frame buffer, erasing
// any previous image there.
//
// A subsequent call to ks_scale_frame() will overwite the image.
void ks_indicate_status(const std::string &message);

// Returns a reference to the scaler subsystem's frame buffer.
//
// The frame buffer contains the most recent image produced by the subsystem.
// This may be a scaled frame (produced by ks_scale_frame()) or some other type
// of image (e.g. one produced by ks_indicate_no_signal()).
image_s ks_scaler_frame_buffer(void);

// Returns a list of the names of the image scalers available in this build of
// VCS.
std::vector<std::string> ks_scaler_names(void);

// Sets the multiplier by which input frames are scaled. The multiplier applies
// to both the width and height of the frame.
void ks_set_scaling_multiplier(const double s);

// Sets the scaler to be used when the resolution of captured frames doesn't match
// the resolution of the output window, such that the frames are scaled to the size
// of the window.
//
// See ks_scaler_names() for a list of available scaler names.
void ks_set_default_scaler(const std::string &name);

// Returns a pointer to the currently-active default scaler.
const image_scaler_s* ks_default_scaler(void);

#endif
