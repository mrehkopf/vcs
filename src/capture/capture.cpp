/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS capture
 *
 * Handles interactions with the capture hardware.
 *
 */

#include <QDebug>
#include <thread>
#include <atomic>
#include <mutex>
#include "../common/command_line.h"
#include "../common/propagate.h"
#include "../display/display.h"
#include "../common/globals.h"
#include "../common/disk.h"
#include "capture.h"

// All local RGBEASY API callbacks lock this for their duration.
std::mutex INPUT_OUTPUT_MUTEX;

// Set to true if the current input resolution is an alias for another resolution.
static bool IS_ALIASED_INPUT_RESOLUTION = false;

// Set to true when receiving the first frame after 'no signal'.
static bool SIGNAL_WOKE_UP = false;

// If set to true, the scaler should skip the next frame we send.
static u32 SKIP_NEXT_NUM_FRAMES = false;

static std::vector<mode_params_s> KNOWN_MODES;

// The color depth/format in which the capture hardware captures the frames.
static PIXELFORMAT CAPTURE_PIXEL_FORMAT = RGB_PIXELFORMAT_888;

// The color depth in which the capture hardware is expected to be sending the
// frames. This depends on the current pixel format, such that e.g. formats
// 555 and 565 are probably sent as 16-bit, and 888 as 32-bit.
static u32 CAPTURE_OUTPUT_COLOR_DEPTH = 32;

// Used to keep track of whether we have new frames to be processed (i.e. if the
// current count of captured frames doesn't equal the number of processed frames).
// Doesn't matter if these counters wrap around.
static std::atomic<unsigned int> CNT_FRAMES_PROCESSED(0);
static std::atomic<unsigned int> CNT_FRAMES_CAPTURED(0);

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and had to skip. Call kc_reset_missed_frames_count() to reset it.
static std::atomic<unsigned int> CNT_FRAMES_SKIPPED(0);

// Whether the capture hardware is receiving a signal from its input.
static std::atomic<bool> RECEIVING_A_SIGNAL(true);

// Will be set to true by the capture hardware callback if the card experiences an
// unrecoverable error.
static bool UNRECOVERABLE_CAPTURE_ERROR = false;

// Set to true if the capture signal is invalid.
static std::atomic<bool> SIGNAL_IS_INVALID(false);

// Will be set to true when the input signal is lost, and back to false once the
// events processor has acknowledged the loss of signal.
static bool SIGNAL_WAS_LOST = false;

// Set to true if the capture hardware reports the current signal as invalid. Will be
// automatically set back to false once the events processor has acknowledged the
// invalidity of the signal.
static bool SIGNAL_BECAME_INVALID = false;

// Frames sent by the capture hardware will be stored here. Note that only one frame
// will fit at a time - if the capture hardware sends in a new frame before the
// previous one has been processed, the new frame will be ignored.
static captured_frame_s FRAME_BUFFER;

// Set to true if the capture hardware's input mode changes.
static bool RECEIVED_NEW_VIDEO_MODE = false;

// The maximum image depth that the capturer can handle.
static const u32 MAX_BIT_DEPTH = 32;

// Set to 1 if we've acquired access to the RGBEASY API.
static bool RGBEASY_IS_LOADED = 0;

static HRGB CAPTURE_HANDLE = 0;
static HRGBDLL RGBAPI_HANDLE = 0;

// Set to 1 if we're currently capturing.
static bool CAPTURE_IS_ACTIVE = false;

// Set to 1 if the input channel we were requested to use was invalid.
static bool INPUT_CHANNEL_IS_INVALID = 0;

// Aliases are resolutions that stand in for others; i.e. if 640 x 480 is an alias
// for 1024 x 768, VCS will ask the capture hardware to switch to 640 x 480 every time
// the card sets 1024 x 768.
static std::vector<mode_alias_s> ALIASES;

// Functions dealing with user-immutable aspects of the capture hardware. These
// will be made available for the entire codebase to use.
static capture_hardware_s CAPTURE_HARDWARE;

// Functions to do with managing the operation of the capture hardware. These
// will remain private to this unit.
static struct capture_interface_s
{
    bool initialize_hardware(void);
    bool release_hardware(void);
    bool start_capture(void);
    bool stop_capture(void);
    bool pause_capture(void);
    bool resume_capture(void);
    bool set_capture_resolution(const resolution_s &r);
} CAPTURE_INTERFACE;

// Callback functions for the RGBEasy API, through which the API communicates
// with VCS.
namespace api_callbacks_n
{
#if !USE_RGBEASY_API
    void frame_captured(void){}
    void video_mode_changed(void){}
    void invalid_signal(void){}
    void no_signal(void){}
    void error(void){}
#else
    // Called by the capture hardware when a new frame has been captured. The
    // captured RGBA data is in frameData.
    void RGBCBKAPI frame_captured(HWND, HRGB, LPBITMAPINFOHEADER frameInfo, void *frameData, ULONG_PTR)
    {
        if (CNT_FRAMES_CAPTURED != CNT_FRAMES_PROCESSED)
        {
            //ERRORI(("The capture hardware sent in a frame before VCS had finished processing "
            //        "the previous one. Skipping this new one. (Captured: %u, processed: %u.)",
            //        CNT_FRAMES_CAPTURED.load(), CNT_FRAMES_PROCESSED.load()));
            CNT_FRAMES_SKIPPED++;
            return;
        }

        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // This could happen e.g. if direct DMA transfer is enabled.
        if (frameData == nullptr ||
                frameInfo == nullptr)
        {
            goto done;
        }

        if (FRAME_BUFFER.pixels.is_null())
        {
            //ERRORI(("The capture hardware sent in a frame but the frame buffer was uninitialized. "
            //        "Ignoring the frame."));
            goto done;
        }

        if (frameInfo->biBitCount > MAX_BIT_DEPTH)
        {
            //ERRORI(("The capture hardware sent in a frame that had an illegal bit depth (%u). "
            //        "The maximum allowed bit depth is %u.", frameInfo->biBitCount, MAX_BIT_DEPTH));
            goto done;
        }

        FRAME_BUFFER.r.w = frameInfo->biWidth;
        FRAME_BUFFER.r.h = abs(frameInfo->biHeight);
        FRAME_BUFFER.r.bpp = frameInfo->biBitCount;

        // Copy the frame's data into our local buffer so we can work on it.
        memcpy(FRAME_BUFFER.pixels.ptr(), (u8*)frameData,
               FRAME_BUFFER.pixels.up_to(FRAME_BUFFER.r.w * FRAME_BUFFER.r.h * (FRAME_BUFFER.r.bpp / 8)));

    done:
        CNT_FRAMES_CAPTURED++;
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI video_mode_changed(HWND, HRGB, PRGBMODECHANGEDINFO info, ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        SIGNAL_WOKE_UP = !RECEIVING_A_SIGNAL;
        RECEIVED_NEW_VIDEO_MODE = true;
        SIGNAL_IS_INVALID = false;

    done:
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI invalid_signal(HWND, HRGB, unsigned long horClock, unsigned long verClock, ULONG_PTR captureHandle)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // Let the card apply its own no signal handler as well, just in case.
        RGBInvalidSignal(captureHandle, horClock, verClock);

        SIGNAL_IS_INVALID = true;

    done:
        return;
    }

    // Called by the capture hardware when no input signal is present.
    void RGBCBKAPI no_signal(HWND, HRGB, ULONG_PTR captureHandle)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Let the card apply its own no signal handler as well, just in case.
        RGBNoSignal(captureHandle);

        SIGNAL_WAS_LOST = true;

        return;
    }

    void RGBCBKAPI error(HWND, HRGB, unsigned long error, ULONG_PTR, unsigned long*)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        UNRECOVERABLE_CAPTURE_ERROR = true;

        return;
    }
#endif
}

// Returns true if the given RGBEase API call return value indicates an error.
bool apicall_succeeds(long callReturnValue)
{
    if (callReturnValue != RGBERROR_NO_ERROR)
    {
        NBENE(("A call to the RGBEasy API returned with error code (0x%x).", callReturnValue));
        return false;
    }

    return true;
}

const capture_hardware_s& kc_hardware(void)
{
    return CAPTURE_HARDWARE;
}

void update_known_mode_params(const resolution_s r,
                              const capture_color_settings_s *const c,
                              const capture_video_settings_s *const v)
{
    uint idx;
    for (idx = 0; idx < KNOWN_MODES.size(); idx++)
    {
        if (KNOWN_MODES[idx].r.w == r.w &&
            KNOWN_MODES[idx].r.h == r.h)
        {
            goto mode_exists;
        }
    }

    // If the mode doesn't already exist, add it.
    KNOWN_MODES.push_back({r,
                           CAPTURE_HARDWARE.meta.default_color_settings(),
                           CAPTURE_HARDWARE.meta.default_video_settings()});

    kd_signal_new_known_mode(r);

    mode_exists:
    // Update the existing mode with the new parameters.
    if (c != nullptr) KNOWN_MODES[idx].color = *c;
    if (v != nullptr) KNOWN_MODES[idx].video = *v;

    return;
}

// Returns true if the capture hardware has been offering frames while the previous
// frame was still being processed for display.
//
bool kc_are_frames_being_missed(void)
{
    return bool(CNT_FRAMES_SKIPPED > 0);
}

uint kc_num_missed_frames(void)
{
    return CNT_FRAMES_SKIPPED;
}

void kc_reset_missed_frames_count(void)
{
    CNT_FRAMES_SKIPPED = 0;

    return;
}

// Creates a test pattern into the frame buffer.
//
void kc_insert_test_image(void)
{
    // For making the test pattern move with successive calls.
    static uint offset = 0;
    offset++;

    for (uint y = 0; y < FRAME_BUFFER.r.h; y++)
    {
        for (uint x = 0; x < FRAME_BUFFER.r.w; x++)
        {
            const uint idx = ((x + y * FRAME_BUFFER.r.w) * 4);
            FRAME_BUFFER.pixels[idx + 0] = (offset+x)%256;
            FRAME_BUFFER.pixels[idx + 1] = (offset+y)%256;
            FRAME_BUFFER.pixels[idx + 2] = 150;
            FRAME_BUFFER.pixels[idx + 3] = 255;
        }
    }

    return;
}

const captured_frame_s& kc_latest_captured_frame(void)
{
    return FRAME_BUFFER;
}

void kc_initialize_capture(void)
{
    INFO(("Initializing capture."));

    FRAME_BUFFER.pixels.alloc(MAX_FRAME_SIZE);

    #ifndef USE_RGBEASY_API
        FRAME_BUFFER.r = {640, 480, 32};

        INFO(("The RGBEASY API is disabled by code. Skipping capture initialization."));
        goto done;
    #endif

    // Open an input on the capture hardware, and have it start sending in frames.
    {
        if (!CAPTURE_INTERFACE.initialize_hardware() ||
            !CAPTURE_INTERFACE.start_capture())
        {
            NBENE(("Failed to initialize capture."));

            PROGRAM_EXIT_REQUESTED = 1;
            goto done;
        }
    }

    // Load previously-saved settings, if any.
    {
        if (!kdisk_load_aliases(kcom_alias_file_name()))
        {
            NBENE(("Failed loading mode aliases from disk.\n"));
            PROGRAM_EXIT_REQUESTED = 1;
            goto done;
        }

        if (!kdisk_load_mode_params(kcom_params_file_name()))
        {
            NBENE(("Failed loading mode parameters from disk.\n"));
            PROGRAM_EXIT_REQUESTED = 1;
            goto done;
        }
    }

    done:
    kpropagate_news_of_new_capture_video_mode();
    return;
}

void kc_set_mode_params(const std::vector<mode_params_s> &modeParams)
{
    KNOWN_MODES = modeParams;

    return;
}

bool kc_adjust_video_vertical_offset(const int delta)
{
    if (!delta) return true;

    const long newPos = (CAPTURE_HARDWARE.status.video_settings().verticalPosition + delta);
    if (newPos < std::max(2, (int)CAPTURE_HARDWARE.meta.minimum_video_settings().verticalPosition) ||
        newPos > CAPTURE_HARDWARE.meta.maximum_video_settings().verticalPosition)
    {
        // ^ Testing for < 2 along with < MIN_VIDEO_PARAMS.verPos, since on my
        // VisionRGB-PRO2, MIN_VIDEO_PARAMS.verPos gives a value less than 2,
        // but setting any such value corrupts the capture.
        return false;
    }

    if (apicall_succeeds(RGBSetVerPosition(CAPTURE_HANDLE, newPos)))
    {
        kd_update_gui_video_params();
    }

    return true;
}

bool kc_adjust_video_horizontal_offset(const int delta)
{
    if (!delta) return true;

    const long newPos = (CAPTURE_HARDWARE.status.video_settings().horizontalPosition + delta);
    if (newPos < CAPTURE_HARDWARE.meta.minimum_video_settings().horizontalPosition ||
        newPos > CAPTURE_HARDWARE.meta.maximum_video_settings().horizontalPosition)
    {
        return false;
    }

    if (apicall_succeeds(RGBSetHorPosition(CAPTURE_HANDLE, newPos)))
    {
        kd_update_gui_video_params();
    }

    return true;
}

void kc_release_capture(void)
{
    INFO(("Releasing the capturer."));

    if (FRAME_BUFFER.pixels.is_null())
    {
        DEBUG(("Was asked to release the capturer, but the framebuffer was null. "
               "maybe the capturer hadn't been initialized? Ignoring this request."));

        return;
    }

    if (CAPTURE_INTERFACE.stop_capture() &&
        CAPTURE_INTERFACE.release_hardware())
    {
        INFO(("The capture hardware has been released."));
    }
    else
    {
        NBENE(("Failed to release the capture hardware."));
    }

    FRAME_BUFFER.pixels.release_memory();

    return;
}

uint kc_input_channel_idx(void)
{
    return INPUT_CHANNEL_IDX;
}

bool kc_is_capture_active(void)
{
    return CAPTURE_IS_ACTIVE;
}

bool kc_set_resolution(const resolution_s r)
{
    return CAPTURE_INTERFACE.set_capture_resolution(r);
}

int kc_index_of_alias_resolution(const resolution_s r)
{
    for (size_t i = 0; i < ALIASES.size(); i++)
    {
        if (ALIASES[i].from.w == r.w &&
            ALIASES[i].from.h == r.h)
        {
            return i;
        }
    }

    // No alias found.
    return -1;
}

mode_params_s kc_mode_params_for_resolution(const resolution_s r)
{
    for (const auto &m: KNOWN_MODES)
    {
        if (m.r.w == r.w &&
            m.r.h == r.h)
        {
            return m;
        }
    }

    INFO(("Unknown video mode; returning default parameters."));
    return {r,
            CAPTURE_HARDWARE.meta.default_color_settings(),
            CAPTURE_HARDWARE.meta.default_video_settings()};
}

bool kc_set_mode_parameters_for_resolution(const resolution_s r)
{
    INFO(("Applying mode parameters for %u x %u.", r.w, r.h));

    mode_params_s p = kc_mode_params_for_resolution(r);

    // Apply the set of mode parameters for the current input resolution.
    /// TODO. Add error-checking.
    RGBSetPhase(CAPTURE_HANDLE,         p.video.phase);
    RGBSetBlackLevel(CAPTURE_HANDLE,    p.video.blackLevel);
    RGBSetHorScale(CAPTURE_HANDLE,      p.video.horizontalScale);
    RGBSetHorPosition(CAPTURE_HANDLE,   p.video.horizontalPosition);
    RGBSetVerPosition(CAPTURE_HANDLE,   p.video.verticalPosition);
    RGBSetBrightness(CAPTURE_HANDLE,    p.color.overallBrightness);
    RGBSetContrast(CAPTURE_HANDLE,      p.color.overallContrast);
    RGBSetColourBalance(CAPTURE_HANDLE, p.color.redBrightness,
                                        p.color.greenBrightness,
                                        p.color.blueBrightness,
                                        p.color.redContrast,
                                        p.color.greenContrast,
                                        p.color.blueContrast);

    return true;
}

bool kc_is_aliased_resolution(void)
{
    return IS_ALIASED_INPUT_RESOLUTION;
}

// See if there isn't an alias resolution for the given resolution.
// If there is, will return that. Otherwise, returns the resolution
// that was passed in.
//
resolution_s aliased(const resolution_s &r)
{
    resolution_s newRes = r;
    const int aliasIdx = kc_index_of_alias_resolution(r);

    if (aliasIdx >= 0)
    {
        newRes = ALIASES[aliasIdx].to;

        // Try to switch to the alias resolution.
        if (!kc_set_resolution(r))
        {
            NBENE(("Failed to apply an alias."));

            IS_ALIASED_INPUT_RESOLUTION = false;
            newRes = r;
        }
        else
        {
            IS_ALIASED_INPUT_RESOLUTION = true;
        }
    }
    else
    {
        IS_ALIASED_INPUT_RESOLUTION = false;
    }

    return newRes;
}

void kc_apply_new_capture_resolution(void)
{
    resolution_s r = aliased(kc_hardware().status.capture_resolution());

    kc_set_mode_parameters_for_resolution(r);

    RECEIVED_NEW_VIDEO_MODE = false;

    INFO(("Capturer reports new input mode: %u x %u.", r.w, r.h));

    return;
}

bool kc_should_current_frame_be_skipped(void)
{
    return bool(SKIP_NEXT_NUM_FRAMES > 0);
}

void kc_mark_current_frame_as_processed(void)
{
    CNT_FRAMES_PROCESSED = CNT_FRAMES_CAPTURED.load();

    if (SKIP_NEXT_NUM_FRAMES > 0)
    {
        SKIP_NEXT_NUM_FRAMES--;
    }

    FRAME_BUFFER.processed = true;

    return;
}

bool kc_is_invalid_signal(void)
{
    return SIGNAL_IS_INVALID;
}

bool kc_no_signal(void)
{
    return !RECEIVING_A_SIGNAL;
}

// Examine the state of the capture system and decide which has been the most recent
// capture event.
//
/// FIXME: This is a bit of an ugly way to handle things. For instance, the function
/// is a getter, but also modifies the unit's state.
capture_event_e kc_latest_capture_event(void)
{
    if (UNRECOVERABLE_CAPTURE_ERROR)
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (RECEIVED_NEW_VIDEO_MODE)
    {
        RECEIVING_A_SIGNAL = true;
        SIGNAL_IS_INVALID = false;

        return capture_event_e::new_video_mode;
    }
    else if (SIGNAL_WAS_LOST)
    {
        RECEIVING_A_SIGNAL = false;
        SIGNAL_WAS_LOST = false;

        return capture_event_e::no_signal;
    }
    else if (!RECEIVING_A_SIGNAL)
    {
        return capture_event_e::sleep;
    }
    else if (SIGNAL_BECAME_INVALID)
    {
        RECEIVING_A_SIGNAL = false;
        SIGNAL_IS_INVALID = true;
        SIGNAL_BECAME_INVALID = false;

        return capture_event_e::invalid_signal;
    }
    else if (SIGNAL_IS_INVALID)
    {
        return capture_event_e::sleep;
    }
    else if (CNT_FRAMES_CAPTURED != CNT_FRAMES_PROCESSED)
    {
        return capture_event_e::new_frame;
    }

    // If there were no events.
    return capture_event_e::none;
}

bool kc_set_frame_dropping(const u32 drop)
{
    // Sanity check.
    k_assert(drop < 100, "Odd frame drop number.");

    if (apicall_succeeds(RGBSetFrameDropping(CAPTURE_HANDLE, drop)))
    {
        INFO(("Setting frame drop to %u.", drop));

        FRAME_SKIP = drop;

        goto success;
    }
    else
    {
        NBENE(("Failed to set frame drop to %u.", drop));
        goto fail;
    }

    success:
    return true;

    fail:
    return false;
}

bool kc_set_input_channel(const u32 channel)
{
    if (channel >= MAX_INPUT_CHANNELS)
    {
        goto fail;
    }

    if (apicall_succeeds(RGBSetInput(CAPTURE_HANDLE, channel)))
    {
        INFO(("Setting capture input channel to %u.", (channel + 1)));

        INPUT_CHANNEL_IDX = channel;
    }
    else
    {
        NBENE(("Failed to set capture input channel to %u.", (channel + 1)));

        goto fail;
    }

    return true;

    fail:
    return false;
}

uint kc_output_color_depth(void)
{
    return CAPTURE_OUTPUT_COLOR_DEPTH;
}

PIXELFORMAT kc_pixel_format(void)
{
    return CAPTURE_PIXEL_FORMAT;
}

uint kc_input_color_depth(void)
{
    switch (CAPTURE_PIXEL_FORMAT)
    {
        case RGB_PIXELFORMAT_888: return 24;
        case RGB_PIXELFORMAT_565: return 16;
        case RGB_PIXELFORMAT_555: return 15;
        default: k_assert(0, "Found an unknown pixel format while being queried for it."); return 0;
    }
}

bool kc_set_input_color_depth(const u32 bpp)
{
    const PIXELFORMAT previousFormat = CAPTURE_PIXEL_FORMAT;
    const uint previousColorDepth = CAPTURE_OUTPUT_COLOR_DEPTH;

    switch (bpp)
    {
        case 24: CAPTURE_PIXEL_FORMAT = RGB_PIXELFORMAT_888; CAPTURE_OUTPUT_COLOR_DEPTH = 32; break;
        case 16: CAPTURE_PIXEL_FORMAT = RGB_PIXELFORMAT_565; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
        case 15: CAPTURE_PIXEL_FORMAT = RGB_PIXELFORMAT_555; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
        default: k_assert(0, "Was asked to set an unknown pixel format."); break;
    }

    if (!apicall_succeeds(RGBSetPixelFormat(CAPTURE_HANDLE, CAPTURE_PIXEL_FORMAT)))
    {
        CAPTURE_PIXEL_FORMAT = previousFormat;
        CAPTURE_OUTPUT_COLOR_DEPTH = previousColorDepth;

        goto fail;
    }

    // Ignore the next frame to avoid displaying some visual corruption from
    // switching the bit depth.
    SKIP_NEXT_NUM_FRAMES += 1;

    return true;

    fail:
    return false;
}

// Lets the gui know which aliases we've got loaded.
void kc_broadcast_aliases_to_gui(void)
{
    DEBUG(("Broadcasting %u alias set(s) to the GUI.", ALIASES.size()));

    kd_clear_known_aliases();
    for (const auto &a: ALIASES)
    {
        kd_signal_new_known_alias(a);
    }

    return;
}

const std::vector<mode_params_s>& kc_mode_params(void)
{
    return KNOWN_MODES;
}

const std::vector<mode_alias_s>& kc_aliases(void)
{
    return ALIASES;
}

void kc_set_aliases(const std::vector<mode_alias_s> &aliases)
{
    ALIASES = aliases;

    if (!kc_no_signal())
    {
        // If one of the aliases matches the current input resolution, change the
        // resolution accordingly.
        const resolution_s currentRes = kc_hardware().status.capture_resolution();
        for (const auto &alias: ALIASES)
        {
            if (alias.from.w == currentRes.w &&
                alias.from.h == currentRes.h)
            {
                kpropagate_forced_capture_resolution(alias.to);
                break;
            }
        }
    }

    return;
}

void kc_broadcast_mode_params_to_gui(void)
{
    kd_clear_known_modes();
    for (const auto &m: KNOWN_MODES)
    {
        kd_signal_new_known_mode(m.r);
    }

    return;
}

void kc_set_color_settings(const capture_color_settings_s c)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to set capture color params while there was no signal. "
               "Ignoring the request."));
        return;
    }

    RGBSetBrightness(CAPTURE_HANDLE, c.overallBrightness);
    RGBSetContrast(CAPTURE_HANDLE, c.overallContrast);
    RGBSetColourBalance(CAPTURE_HANDLE, c.redBrightness,
                                        c.greenBrightness,
                                        c.blueBrightness,
                                        c.redContrast,
                                        c.greenContrast,
                                        c.blueContrast);

    update_known_mode_params(kc_hardware().status.capture_resolution(), &c, nullptr);

    return;
}

void kc_set_video_settings(const capture_video_settings_s v)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));
        return;
    }

    RGBSetPhase(CAPTURE_HANDLE, v.phase);
    RGBSetBlackLevel(CAPTURE_HANDLE, v.blackLevel);
    RGBSetHorPosition(CAPTURE_HANDLE, v.horizontalPosition);
    RGBSetHorScale(CAPTURE_HANDLE, v.horizontalScale);
    RGBSetVerPosition(CAPTURE_HANDLE, v.verticalPosition);

    update_known_mode_params(kc_hardware().status.capture_resolution(), nullptr, &v);

    return;
}

#ifdef VALIDATION_RUN
    void kc_VALIDATION_set_capture_color_depth(const uint bpp)
    {
        CAPTURE_COLOR_DEPTH = bpp;
        return;
    }

    void kc_VALIDATION_set_capture_pixel_format(const PIXELFORMAT pf)
    {
        CAPTURE_PIXEL_FORMAT = pf;
        return;
    }
#endif

bool capture_hardware_s::features_supported_s::component_capture() const
{
    for (uint i = 0; i < MAX_INPUT_CHANNELS; i++)
    {
        long isSupported = 0;
        if (!apicall_succeeds(RGBInputIsComponentSupported(i, &isSupported))) return false;
        if (isSupported) return true;
    }
    return false;
}

bool capture_hardware_s::features_supported_s::composite_capture() const
{
    long isSupported = 0;
    for (uint i = 0; i < MAX_INPUT_CHANNELS; i++)
    {
        if (!apicall_succeeds(RGBInputIsCompositeSupported(i, &isSupported))) return false;
        if (isSupported) return true;
    }
    return false;
}

bool capture_hardware_s::features_supported_s::deinterlace() const
{
    long isSupported = 0;
    if (!apicall_succeeds(RGBIsDeinterlaceSupported(&isSupported))) return false;
    return isSupported;
}

bool capture_hardware_s::features_supported_s::dma() const
{
    long isSupported = 0;
    if (!apicall_succeeds(RGBIsDirectDMASupported(&isSupported))) return false;
    return isSupported;
}

bool capture_hardware_s::features_supported_s::dvi() const
{
    for (uint i = 0; i < MAX_INPUT_CHANNELS; i++)
    {
        long isSupported = 0;
        if (!apicall_succeeds(RGBInputIsDVISupported(i, &isSupported))) return false;
        if (isSupported) return true;
    }
    return false;
}

bool capture_hardware_s::features_supported_s::svideo() const
{
    for (uint i = 0; i < MAX_INPUT_CHANNELS; i++)
    {
        long isSupported = 0;
        if (!apicall_succeeds(RGBInputIsSVideoSupported(i, &isSupported))) return false;
        if (isSupported) return true;
    }
    return false;
}

bool capture_hardware_s::features_supported_s::vga() const
{
    for (uint i = 0; i < MAX_INPUT_CHANNELS; i++)
    {
        long isSupported = 0;
        if (!apicall_succeeds(RGBInputIsVGASupported(i, &isSupported))) return false;
        if (isSupported) return true;
    }
    return false;
}

bool capture_hardware_s::features_supported_s::yuv() const
{
    long isSupported = 0;
    if (!apicall_succeeds(RGBIsYUVSupported(&isSupported))) return false;
    return isSupported;
}

std::string capture_hardware_s::metainfo_s::model_name() const
{
    const std::string unknownName = "Unknown capture device";

    CAPTURECARD card = RGB_CAPTURECARD_DGC103;
    if (!apicall_succeeds(RGBGetCaptureCard(&card))) return unknownName;

    switch (card)
    {
        case RGB_CAPTURECARD_DGC103: return "Datapath VisionRGB-PRO";
        case RGB_CAPTURECARD_DGC133: return "Datapath DGC133 Series";
        default: break;
    }

    return unknownName;
}

int capture_hardware_s::metainfo_s::minimum_frame_drop() const
{
    unsigned long frameDrop = 0;
    if (!apicall_succeeds(RGBGetFrameDroppingMinimum(CAPTURE_HANDLE, &frameDrop))) return -1;
    return frameDrop;
}

int capture_hardware_s::metainfo_s::maximum_frame_drop() const
{
    unsigned long frameDrop = 0;
    if (!apicall_succeeds(RGBGetFrameDroppingMaximum(CAPTURE_HANDLE, &frameDrop))) return -1;
    return frameDrop;
}

std::string capture_hardware_s::metainfo_s::driver_version() const
{
    const std::string unknownVersion = "Unknown";

    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeds(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii))) return unknownVersion;

    return std::string(std::to_string(ii.Driver.Major) + "." +
                       std::to_string(ii.Driver.Minor) + "." +
                       std::to_string(ii.Driver.Micro) + "/" +
                       std::to_string(ii.Driver.Revision));
}

std::string capture_hardware_s::metainfo_s::firmware_version() const
{
    const std::string unknownVersion = "Unknown";

    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeds(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii))) return unknownVersion;

    return std::to_string(ii.FirmWare);
}

capture_color_settings_s capture_hardware_s::metainfo_s::default_color_settings() const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeds(RGBGetBrightnessDefault(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeds(RGBGetContrastDefault(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeds(RGBGetColourBalanceDefault(CAPTURE_HANDLE, &p.redBrightness,
                                                                     &p.greenBrightness,
                                                                     &p.blueBrightness,
                                                                     &p.redContrast,
                                                                     &p.greenContrast,
                                                                     &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

capture_color_settings_s capture_hardware_s::metainfo_s::minimum_color_settings() const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeds(RGBGetBrightnessMinimum(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeds(RGBGetContrastMinimum(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeds(RGBGetColourBalanceMinimum(CAPTURE_HANDLE, &p.redBrightness,
                                                                     &p.greenBrightness,
                                                                     &p.blueBrightness,
                                                                     &p.redContrast,
                                                                     &p.greenContrast,
                                                                     &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

capture_color_settings_s capture_hardware_s::metainfo_s::maximum_color_settings() const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeds(RGBGetBrightnessMaximum(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeds(RGBGetContrastMaximum(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeds(RGBGetColourBalanceMaximum(CAPTURE_HANDLE, &p.redBrightness,
                                                                     &p.greenBrightness,
                                                                     &p.blueBrightness,
                                                                     &p.redContrast,
                                                                     &p.greenContrast,
                                                                     &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_hardware_s::metainfo_s::default_video_settings() const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeds(RGBGetPhaseDefault(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeds(RGBGetBlackLevelDefault(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeds(RGBGetHorPositionDefault(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeds(RGBGetVerPositionDefault(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeds(RGBGetHorScaleDefault(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_hardware_s::metainfo_s::minimum_video_settings() const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeds(RGBGetPhaseMinimum(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeds(RGBGetBlackLevelMinimum(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeds(RGBGetHorPositionMinimum(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeds(RGBGetVerPositionMinimum(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeds(RGBGetHorScaleMinimum(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_hardware_s::metainfo_s::maximum_video_settings() const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeds(RGBGetPhaseMaximum(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeds(RGBGetBlackLevelMaximum(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeds(RGBGetHorPositionMaximum(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeds(RGBGetVerPositionMaximum(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeds(RGBGetHorScaleMaximum(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

resolution_s capture_hardware_s::metainfo_s::minimum_capture_resolution() const
{
    resolution_s r;

#if !USE_RGBEASY_API
    r.w = 1;
    r.h = 1;
#else
    if (!apicall_succeeds(RGBGetCaptureWidthMinimum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeds(RGBGetCaptureHeightMinimum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }
#endif

    // NOTE: It's assumed that 16-bit is the minimum capture color depth.
    r.bpp = 16;

    return r;
}

resolution_s capture_hardware_s::metainfo_s::maximum_capture_resolution() const
{
    resolution_s r;

#if !USE_RGBEASY_API
    r.w = 1920;
    r.h = 1260;
#else
    if (!apicall_succeeds(RGBGetCaptureWidthMaximum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeds(RGBGetCaptureHeightMaximum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }
#endif

    // NOTE: It's assumed that 32-bit is the maximum capture color depth.
    r.bpp = 32;

    return r;
}

int capture_hardware_s::metainfo_s::num_capture_inputs() const
{
    unsigned long numInputs = 0;
    if (!apicall_succeeds(RGBGetNumberOfInputs(&numInputs))) return -1;
    return numInputs;
}

bool capture_hardware_s::metainfo_s::is_dma_enabled() const
{
    long isEnabled = 0;
    if (!apicall_succeeds(RGBGetDMADirect(CAPTURE_HANDLE, &isEnabled))) return false;
    return isEnabled;
}

resolution_s capture_hardware_s::status_s::capture_resolution() const
{
    resolution_s r;

#if USE_RGBEASY_API
    if (!apicall_succeeds(RGBGetCaptureWidth(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeds(RGBGetCaptureHeight(CAPTURE_HANDLE, &r.h)))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }
#else
    r.w = 640;
    r.h = 480;
#endif

    r.bpp = CAPTURE_OUTPUT_COLOR_DEPTH;

    return r;
}

capture_color_settings_s capture_hardware_s::status_s::color_settings() const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeds(RGBGetBrightness(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeds(RGBGetContrast(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeds(RGBGetColourBalance(CAPTURE_HANDLE, &p.redBrightness,
                                                              &p.greenBrightness,
                                                              &p.blueBrightness,
                                                              &p.redContrast,
                                                              &p.greenContrast,
                                                              &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_hardware_s::status_s::video_settings() const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeds(RGBGetPhase(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeds(RGBGetBlackLevel(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeds(RGBGetHorPosition(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeds(RGBGetVerPosition(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeds(RGBGetHorScale(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_signal_s capture_hardware_s::status_s::signal() const
{
    if (kc_no_signal())
    {
        NBENE(("Tried to query the capture signal while no signal was being received."));
        return {0};
    }

    capture_signal_s s = {0};

#if !USE_RGBEASY_API
    s.r.w = 640;
    s.r.h = 480;
    s.refreshRate = 60;
#else
    RGBMODEINFO mi = {0};
    mi.Size = sizeof(mi);

    s.wokeUp = SIGNAL_WOKE_UP;

    if (apicall_succeeds(RGBGetModeInfo(CAPTURE_HANDLE, &mi)))
    {
        s.isInterlaced = mi.BInterlaced;
        s.isDigital = mi.BDVI;
        s.refreshRate = round(mi.RefreshRate / 1000.0);
    }
    else
    {
        s.isInterlaced = 0;
        s.isDigital = false;
        s.refreshRate = 0;
    }

    s.r = CAPTURE_HARDWARE.status.capture_resolution();
#endif

    return s;
}

int capture_hardware_s::status_s::frame_rate() const
{
    unsigned long rate = 0;
    if (!apicall_succeeds(RGBGetFrameRate(CAPTURE_HANDLE, &rate))) return -1;
    return rate;
}

bool capture_interface_s::initialize_hardware()
{
    INFO(("Initializing the capture hardware."));

    if (INPUT_CHANNEL_IDX >= MAX_INPUT_CHANNELS)
    {
        NBENE(("The requested input channel %u is out of bounds.", INPUT_CHANNEL_IDX));
        INPUT_CHANNEL_IS_INVALID = true;
        goto fail;
    }

    if (!apicall_succeeds(RGBLoad(&RGBAPI_HANDLE)))
    {
        goto fail;
    }
    else RGBEASY_IS_LOADED = true;

    if (!apicall_succeeds(RGBOpenInput(INPUT_CHANNEL_IDX, &CAPTURE_HANDLE)) ||
        !apicall_succeeds(RGBSetFrameDropping(CAPTURE_HANDLE, FRAME_SKIP)) ||
        !apicall_succeeds(RGBSetDMADirect(CAPTURE_HANDLE, FALSE)) ||
        !apicall_succeeds(RGBSetPixelFormat(CAPTURE_HANDLE, CAPTURE_PIXEL_FORMAT)) ||
        !apicall_succeeds(RGBUseOutputBuffers(CAPTURE_HANDLE, FALSE)) ||
        !apicall_succeeds(RGBSetFrameCapturedFn(CAPTURE_HANDLE, api_callbacks_n::frame_captured, 0)) ||
        !apicall_succeeds(RGBSetModeChangedFn(CAPTURE_HANDLE, api_callbacks_n::video_mode_changed, 0)) ||
        !apicall_succeeds(RGBSetInvalidSignalFn(CAPTURE_HANDLE, api_callbacks_n::invalid_signal, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeds(RGBSetErrorFn(CAPTURE_HANDLE, api_callbacks_n::error, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeds(RGBSetNoSignalFn(CAPTURE_HANDLE, api_callbacks_n::no_signal, (ULONG_PTR)&CAPTURE_HANDLE)))
    {
        NBENE(("Failed to initialize the capture hardware."));
        goto fail;
    }

    /// Temp hack. We've only allocated enough room in the input frame buffer to
    /// hold at most the maximum output size.
    {
        const resolution_s maxCaptureRes = CAPTURE_HARDWARE.meta.maximum_capture_resolution();
        k_assert((maxCaptureRes.w <= MAX_OUTPUT_WIDTH &&
                  maxCaptureRes.h <= MAX_OUTPUT_HEIGHT),
                 "The capture hardware is not compatible with this version of VCS.");
    }

    return true;

fail:
    return false;
}

bool capture_interface_s::release_hardware()
{
    if (!RGBEASY_IS_LOADED) return true;

    if (!apicall_succeeds(RGBCloseInput(CAPTURE_HANDLE)) ||
            !apicall_succeeds(RGBFree(CAPTURE_HANDLE)))
    {
        return false;
    }

    RGBEASY_IS_LOADED = false;
    return true;
}

bool capture_interface_s::start_capture()
{
    INFO(("Starting capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (RGBStartCapture(CAPTURE_HANDLE) != RGBERROR_NO_ERROR)
    {
        NBENE(("Failed to start capture on input channel %u.", (INPUT_CHANNEL_IDX + 1)));
        goto fail;
    }
    else
    {
        CAPTURE_IS_ACTIVE = true;
    }

    return true;

fail:
    return false;
}

bool capture_interface_s::stop_capture()
{
    INFO(("Stopping capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (CAPTURE_IS_ACTIVE)
    {
        if (!apicall_succeeds(RGBStopCapture(CAPTURE_HANDLE)))
        {
            NBENE(("Failed to stop capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));
            goto fail;
        }

        CAPTURE_IS_ACTIVE = false;
    }
    else
    {
        CAPTURE_IS_ACTIVE = false;

        DEBUG(("Was asked to stop the capture even though it hadn't been started. Ignoring this request."));
    }

    INFO(("Restoring default callback handlers."));
    RGBSetFrameCapturedFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetModeChangedFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetInvalidSignalFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetNoSignalFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetErrorFn(CAPTURE_HANDLE, NULL, 0);

    return true;

fail:
    return false;
}

bool capture_interface_s::pause_capture()
{
    INFO(("Pausing the capture."));
    return apicall_succeeds(RGBPauseCapture(CAPTURE_HANDLE));
}

bool capture_interface_s::resume_capture()
{
    INFO(("Resuming the capture."));
    return apicall_succeeds(RGBResumeCapture(CAPTURE_HANDLE));
}

bool capture_interface_s::set_capture_resolution(const resolution_s &r)
{
    if (!CAPTURE_IS_ACTIVE)
    {
        INFO(("Was asked to set the capture resolution while capture was inactive. Ignoring this request."));
        return false;
    }

    unsigned long wd = 0, hd = 0;

    const auto currentInputRes = kc_hardware().status.capture_resolution();
    if (r.w == currentInputRes.w &&
        r.h == currentInputRes.h)
    {
        DEBUG(("Was asked to force a capture resolution that had already been set. Ignoring the request."));
        goto fail;
    }

    // Test whether the capture hardware can handle the given resolution.
    if (!apicall_succeeds(RGBTestCaptureWidth(CAPTURE_HANDLE, r.w)))
    {
        NBENE(("Failed to force the new input resolution (%u x %u). The capture hardware says the width "
               "is illegal.", r.w, r.h));
        goto fail;
    }

    // Set the new resolution.
    if (!apicall_succeeds(RGBSetCaptureWidth(CAPTURE_HANDLE, (unsigned long)r.w)) ||
        !apicall_succeeds(RGBSetCaptureHeight(CAPTURE_HANDLE, (unsigned long)r.h)) ||
        !apicall_succeeds(RGBSetOutputSize(CAPTURE_HANDLE, (unsigned long)r.w, (unsigned long)r.h)))
    {
        NBENE(("The capture hardware could not properly initialize the new input resolution (%u x %u).",
                r.w, r.h));
        goto fail;
    }

    /// Temp hack.
    RGBGetOutputSize(CAPTURE_HANDLE, &wd, &hd);
    if (wd != r.w ||
        hd != r.h)
    {
        NBENE(("The capture hardware failed to set the desired resolution."));
        goto fail;
    }

    SKIP_NEXT_NUM_FRAMES += 2;  // Avoid garbage on screen while the mode changes.

    return true;

    fail:
    return false;
}
