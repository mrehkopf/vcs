/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
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
 *   1. Call ks_initialize_scaler() to initialize the subsystem. This is VCS's
 *      default startup behavior.
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
 *      kc_evNewCapturedFrame.listen([](const captured_frame_s &frame)
 *      {
 *          ks_scale_frame(frame);
 *      });
 * 
 *      // Executed each time the scaler subsystem produces a new scaled image.
 *      ks_evNewScaledImage.listen([](const captured_frame_s &frame)
 *      {
 *          printf("A frame was scaled to %lu x %lu.\n", frame.r.w, frame.r.h);
 *      });
 *      @endcode
 *
 *   5. Call ks_release_scaler() to release the subsystem. This is VCS's default
 *      exit behavior.
 * 
 */

#ifndef VCS_SCALER_SCALER_H
#define VCS_SCALER_SCALER_H

#include "common/globals.h"
#include "common/propagate/vcs_event.h"

struct captured_frame_s;

/*!
 * An event fired when the scaler subsystem scales a frame into a resolution
 * different from the previous frame's.
 * 
 * @code
 * ks_scale_frame(frame);
 * 
 * ks_set_scaling_multiplier(ks_scaling_multiplier() + 1);
 * 
 * // Fires ks_evNewOutputResolution.
 * ks_scale_frame(frame);
 * @endcode
 */
extern vcs_event_c<const resolution_s&> ks_evNewOutputResolution;

/*!
 * An event fired when the scaler subsystem has finished scaling a frame.
 * 
 * @code
 * ks_evNewScaledImage.listen([](const captured_frame_s &scaledImage)
 * {
 *     printf("Scaled a frame to %lu x %lu.\n", scaledImage.r.w, scaledImage.r.h);
 * });
 * @endcode
 * 
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<const captured_frame_s&> ks_evNewScaledImage;

/*!
 * An event fired once per second, giving the number of input frames the scaler
 * subsystem scaled during that time.
 * 
 * @code
 * ks_evFramesPerSecond.listen([](unsigned numFrames)
 * {
 *     printf("Scaled %u frames per second.\n", numFrames);
 * });
 * @endcode
 * 
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<unsigned> ks_evFramesPerSecond;

/*!
 * Enumerates the aspect ratios recognized by the scaler subsystem.
 * 
 * @see
 * ks_set_aspect_ratio()
 */
enum class scaler_aspect_ratio_e
{
    /*! An image's native aspect ratio (width / height).*/
    native,

    /*!
     * An image's native aspect ratio (width / height), except for certain
     * historically 4:3 aspect ratio resolutions like 720 x 400.
     */
    traditional_4_3,

    /*! An aspect ratio of 4:3, disregarding the image's native ratio.*/
    all_4_3
};

/*!
 * Returns the resolution to which the scaler will scale input frames, prior to
 * the application of resolution-influencing modifiers like a scaling multiplier.
 * 
 * In most cases, you'd be interested in ks_output_resolution(), instead.
 * 
 * @see
 * ks_output_resolution(), ks_scale_frame()
 */
resolution_s ks_base_resolution(void);

/*!
 * Returns the resolution to which the scaler would currently scale an input frame.
 *
 * @see
 * ks_scale_frame(), ks_base_resolution
 */
resolution_s ks_output_resolution(void);

/*!
 * Returns true if the scaler is currently forcing scaled frames into a particular
 * aspect ratio.
 * 
 * @see
 * ks_set_aspect_ratio()
 */
bool ks_is_aspect_ratio_enabled(void);

/*!
 * Initializes the scaler subsystem.
 * 
 * @note
 * This function should be called before any other functions of the scaler interface.
 * 
 * @see
 * ks_release_scaler()
 */
void ks_initialize_scaler(void);

/*!
 * Releases the scaler subsystem, deallocating any of its memory buffers etc.
 * 
 * @warning
 * Between calling this function and ks_initialize_scaler(), no other scaler
 * subsystem functions should be called.
 * 
 * @see
 * ks_initialize_scaler()
 */
void ks_release_scaler(void);

/*!
 * Applies scaling to the given frame's pixels and stores the result in the scaler
 * subsystem's frame buffer. The input data are not modified.
 * 
 * After this call, the scaled image is available via ks_frame_buffer().
 * 
 * @code
 * ks_scale_frame(frame);
 * const auto &scaledFrame = ks_frame_buffer();
 * @endcode
 * 
 * @code
 * // Feed captured frames into the scaler using a VCS event listener.
 * kc_evNewCapturedFrame.listen([](const captured_frame_s &frame)
 * {
 *     ks_scale_frame(frame);
 * });
 * 
 * // Receive scaled frames from the scaler.
 * ks_evNewScaledImage.listen([](const captured_frame_s &frame)
 * {
 *     printf("A frame was scaled to %lu x %lu.\n", frame.r.w, frame.r.h);
 * });
 * @endcode
 * 
 * @see
 * ks_frame_buffer(), ks_evNewScaledImage
 */
void ks_scale_frame(const captured_frame_s &frame);

/*!
 * Sets the aspect ratio to which the scaler subsystem scales input frames.
 * 
 * When scaling to a particular aspect ratio, input frames are squished to the
 * given ratio, then padded with black borders as necessary to fit the output
 * size.
 * 
 * @code
 * // A frame of size 640 x 400 (16:10 aspect ratio).
 * auto frame = ...;
 * 
 * ks_set_base_resolution({640, 400});
 * ks_set_aspect_ratio(scaler_aspect_ratio_e::all_4_3);
 * ks_scale_frame(frame);
 * 
 * // The scaled frame will have been padded to 640 x 400, with the original
 * // frame's pixels squished to 533 x 400 (4:3 aspect ratio).
 * auto scaledFrame = ks_frame_buffer();
 * @endcode
 * 
 * @see
 * ks_aspect_ratio(), ks_set_aspect_ratio_enabled()
 */
void ks_set_aspect_ratio(const scaler_aspect_ratio_e aspect);

/*!
 * Enables or disables the scaler subsystem's aspect ratio.
 * 
 * If disabled, input frames will be scaled to their native aspect ratios.
 */
void ks_set_aspect_ratio_enabled(const bool state);

/*!
 * Returns the scaler subsystem's current aspect ratio.
 * 
 * @see
 * ks_set_aspect_ratio(), ks_set_aspect_ratio_enabled()
 */
scaler_aspect_ratio_e ks_aspect_ratio(void);

/*!
 * Returns the scaler subsystem's current scaling multiplier.
 * 
 * @see
 * ks_set_scaling_multiplier(), ks_set_scaling_multiplier_enabled()
 */
scaler_aspect_ratio_e ks_set_scaling_multiplier(void);

/*!
 * Enables or disables the scaler subsystem's scaling multiplier.
 * 
 * @see
 * ks_set_scaling_multiplier()
 */
void ks_set_scaling_multiplier_enabled(const bool enabled);

/*!
 * Sets the resolution to which input frames are to be scaled, before applying
 * size modifiers like aspect ratio correction or a scaling multiplier. By default,
 * the scaler will apply the modifiers to each input frame's own resolution.
 * 
 * @code
 * // Frames will be scaled to 2x of their original resolution.
 * ks_set_base_resolution_enabled(false);
 * ks_set_scaling_multiplier(2);
 * 
 * // Frames will be scaled to 2x of 640 x 480.
 * ks_set_base_resolution_enabled(true);
 * ks_set_base_resolution({640, 480});
 * ks_set_scaling_multiplier(2);
 * @endcode
 * 
 * @see
 * ks_set_base_resolution_enabled()
 */
void ks_set_base_resolution(const resolution_s &r);

/*!
 * Enables or disables the scaler subsystem's overridable base resolution.
 * 
 * If disabled, the base resolution will be the resolution of the input frame.
 * 
 * @see
 * ks_set_base_resolution()
 */
void ks_set_base_resolution_enabled(const bool enabled);

/*!
 * Draws a "no signal" image into the scaler subsystem's frame buffer, erasing
 * any previous image there.
 * 
 * @note
 * A subsequent call to ks_scale_frame() will overwite the image.
 * 
 * @code
 * // Produce a "no signal" image when the capture device loses its signal.
 * kc_evSignalLost.listen(ks_indicate_no_signal);
 * 
 * // Note: The kc_evSignalLost event fires when the capture device loses its
 * // signal, but not in the case where the device already has no signal when
 * // VCS is launched.
 * @endcode
 * 
 * @see
 * kc_evSignalLost
 */
void ks_indicate_no_signal(void);

/*!
 * Draws an "invalid signal" image into the scaler subsystem's frame buffer,
 * erasing any previous image there.
 * 
 * @note
 * A subsequent call to ks_scale_frame() will overwite the image.
 * 
 * @code
 * // Produce an "invalid signal" image when the capture device loses its signal.
 * kc_evInvalidSignal.listen(ks_indicate_invalid_signal);
 * @endcode
 * 
 * @see
 * kc_evInvalidSignal
 */
void ks_indicate_invalid_signal(void);

/*!
 * Returns a reference to the scaler subsystem's frame buffer.
 * 
 * The frame buffer contains the most recent image produced by the subsystem.
 * This may be a scaled frame (produced by ks_scale_frame()) or some other type
 * of image (e.g. one produced by ks_indicate_no_signal()).
 * 
 * @see
 * ks_scale_frame(), ks_indicate_no_signal(), ks_indicate_invalid_signal()
 */
const captured_frame_s& ks_frame_buffer(void);

/*!
 * Returns the name of the current upscaling filter.
 * 
 * @see
 * ks_set_upscaling_filter(), ks_scaling_filter_names()
 */
const std::string &ks_upscaling_filter_name(void);

/*!
 * Returns the name of the current downscaling filter.
 * 
 * @see
 * ks_set_downscaling_filter(), ks_scaling_filter_names()
 */
const std::string& ks_downscaling_filter_name(void);

/*!
 * Returns a list of the names of the scaling filters available in this build of
 * VCS.
 * 
 * @code
 * const auto list = ks_scaling_filter_names();
 * // list == {"Nearest", "Linear", "Area", ...}.
 * @endcode
 * 
 * @note
 * The presence or absence of the USE_OPENCV build flag affects the availability
 * of scaling filters.
 */
std::vector<std::string> ks_scaling_filter_names(void);

/*!
 * Sets the multiplier by which input frames are scaled. The multiplier applies
 * to both the width and height of the frame.
 * 
 * @code
 * // Input frames will be upscaled by 2x using the current upscaling filter.
 * ks_set_scaling_multiplier(2);
 * 
 * // Input frames will be downscaled by half using the current downscaling filter.
 * ks_set_scaling_multiplier(0.5);
 * @endcode
 * 
 * @see
 * ks_scale_frame(), ks_set_downscaling_filter(), ks_set_upscaling_filter()
 */
void ks_set_scaling_multiplier(const double s);

/*!
 * Sets the scaling filter to be used when frames are downscaled, i.e. when the
 * scaling multiplier is < 1. The desired filter is identified by @p name.
 * 
 * @code
 * // Produce an image downscaled by 0.75x using a linearly sampling scaler.
 * ks_set_downscaling_filter("Linear");
 * ks_set_scaling_multiplier(0.75);
 * ks_scale_frame(frame);
 * 
 * const auto &scaledImage = ks_frame_buffer();
 * @endcode
 * 
 * @note
 * @p name must be one of the strings returned by ks_scaling_filter_names().
 * 
 * @see
 * ks_scaling_filter_names(), ks_set_scaling_multiplier(),
 * ks_downscaling_filter_name()
 */
void ks_set_downscaling_filter(const std::string &name);

/*!
 * Sets the scaling filter to be used when frames are upscaled, i.e. when the
 * scaling multiplier is > 1. The desired filter is identified by @p name.
 * 
 * @code
 * // Produce an image upscaled by 1.25x using a linearly sampling scaler.
 * ks_set_upscaling_filter("Linear");
 * ks_set_scaling_multiplier(1.25);
 * ks_scale_frame(frame);
 * 
 * const auto &scaledImage = ks_frame_buffer();
 * @endcode
 * 
 * @note
 * @p name must be one of the strings returned by ks_scaling_filter_names().
 * 
 * @see
 * ks_scaling_filter_names() ks_set_scaling_multiplier(),
 * ks_upscaling_filter_name()
 */
void ks_set_upscaling_filter(const std::string &name);

#endif
