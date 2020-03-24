#include <atomic>
#include <mutex>
#include <cmath>
#include "common/globals.h"
#include "common/propagate.h"
#include "capture/capture_api_rgbeasy.h"
#include "capture/capture.h"
#include "capture/alias.h"

// A mutex to direct access to data between the main thead and the RGBEasy
// callback thread. For instance, the RGBEasy thread will lock this while
// it's uploading frame data, and the main thread will lock this while it's
// processing said data.
static std::mutex RGBEASY_CALLBACK_MUTEX;

// Frames sent by the capture hardware will be stored here for processing.
static captured_frame_s FRAME_BUFFER;

// The corresponding flag will be set to true when the RGBEasy callback
// thread notifies us of such an event; and reset back to false when we've
// handled that event.
static bool RGBEASY_CAPTURE_EVENT_FLAGS[static_cast<int>(capture_event_e::num_enumerators)] = {false};

// If > 0, the next n captured frames should be ignored by the rest of VCS.
// This can be useful e.g. to avoid displaying the one or two garbles frames
// that may occur when changing video modes.
static unsigned SKIP_NEXT_NUM_FRAMES = 0;

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and so which we had to skip.
static std::atomic<unsigned int> NUM_NEW_FRAME_EVENTS_SKIPPED(0);

static std::vector<video_mode_params_s> KNOWN_MODES;

// The pixel format in which the capture device sends captured frames.
static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

// The color depth in which the capture hardware is expected to be sending the
// frames. This depends on the current pixel format, such that e.g. formats
// 555 and 565 are probably sent as 16-bit, and 888 as 32-bit.
static unsigned CAPTURE_OUTPUT_COLOR_DEPTH = 32;

// Whether the capture hardware is receiving a signal from its input.
static bool RECEIVING_A_SIGNAL = true;

// If the current signal we're receiving is invalid.
static bool IS_SIGNAL_INVALID = false;

// Set to true if the capture hardware's input mode changes.
static bool RECEIVED_NEW_VIDEO_MODE = false;

// The maximum image depth that the capturer can handle.
static const unsigned MAX_BIT_DEPTH = 32;

static HRGB CAPTURE_HANDLE = 0;
static HRGBDLL RGBAPI_HANDLE = 0;

// Set to 1 if we're currently capturing.
static bool CAPTURE_IS_ACTIVE = false;

// Marks the given capture event as having occurred.
static void push_capture_event(capture_event_e event)
{
    RGBEASY_CAPTURE_EVENT_FLAGS[static_cast<int>(event)] = true;

    return;
}

// Removes the given capture event from the 'queue', and returns its value.
static bool pop_capture_event(capture_event_e event)
{
    const bool eventOccurred = RGBEASY_CAPTURE_EVENT_FLAGS[static_cast<int>(event)];
    RGBEASY_CAPTURE_EVENT_FLAGS[static_cast<int>(event)] = false;

    return eventOccurred;
}

// Callback functions for the RGBEasy API, through which the API communicates
// with VCS. RGBEasy isn't supported on platforms other than Windows, hence the
// #if - on other platforms, we load in empty placeholder functions (elsewhere
// in the code).
#if _WIN32
    // Called by the capture hardware when a new frame has been captured. The
    // captured RGBA data is in frameData.
    void RGBCBKAPI rgbeasy_callback__frame_captured(HWND, HRGB, LPBITMAPINFOHEADER frameInfo, void *frameData, ULONG_PTR)
    {
        // If the hardware is sending us a new frame while we're still unfinished
        // with processing the previous frame. In that case, we'll need to skip
        // this new frame.
        if (!RGBEASY_CALLBACK_MUTEX.try_lock())
        {
            NUM_NEW_FRAME_EVENTS_SKIPPED++;
            return;
        }

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

        push_capture_event(capture_event_e::new_frame);

        done:
        RGBEASY_CALLBACK_MUTEX.unlock();
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI rgbeasy_callback__video_mode_changed(HWND, HRGB, PRGBMODECHANGEDINFO, ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(RGBEASY_CALLBACK_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        push_capture_event(capture_event_e::new_video_mode);

        IS_SIGNAL_INVALID = false;
        RECEIVING_A_SIGNAL = true;

        done:
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI rgbeasy_callback__invalid_signal(HWND, HRGB, unsigned long horClock, unsigned long verClock, ULONG_PTR captureHandle)
    {
        std::lock_guard<std::mutex> lock(RGBEASY_CALLBACK_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // Let the card apply its own no signal handler as well, just in case.
        RGBInvalidSignal(captureHandle, horClock, verClock);

        push_capture_event(capture_event_e::invalid_signal);

        IS_SIGNAL_INVALID = true;
        RECEIVING_A_SIGNAL = false;

    done:
        return;
    }

    // Called by the capture hardware when no input signal is present.
    void RGBCBKAPI rgbeasy_callback__no_signal(HWND, HRGB, ULONG_PTR captureHandle)
    {
        std::lock_guard<std::mutex> lock(RGBEASY_CALLBACK_MUTEX);

        // Let the card apply its own 'no signal' handler as well, just in case.
        RGBNoSignal(captureHandle);

        push_capture_event(capture_event_e::no_signal);

        RECEIVING_A_SIGNAL = false;

        return;
    }

    void RGBCBKAPI rgbeasy_callback__error(HWND, HRGB, unsigned long, ULONG_PTR, unsigned long*)
    {
        std::lock_guard<std::mutex> lock(RGBEASY_CALLBACK_MUTEX);

        push_capture_event(capture_event_e::unrecoverable_error);

        RECEIVING_A_SIGNAL = false;

        return;
    }
#endif

bool capture_api_rgbeasy_s::initialize(void)
{
    INFO(("Initializing the capture API."));

    FRAME_BUFFER.pixels.alloc(MAX_FRAME_SIZE);

    // Open an input on the capture hardware, and have it start sending in frames.
    {
        if (!this->initialize_hardware() ||
            !this->start_capture())
        {
            NBENE(("Failed to initialize the capture API."));

            PROGRAM_EXIT_REQUESTED = 1;
            goto done;
        }
    }

    kpropagate_news_of_new_capture_video_mode();

    done:
    // We can successfully exit if the initialization didn't trigger a request
    // to terminate the program.
    return !PROGRAM_EXIT_REQUESTED;
}

bool capture_api_rgbeasy_s::release(void)
{
    INFO(("Releasing the capture API."));

    FRAME_BUFFER.pixels.release_memory();

    if (this->stop_capture() &&
        this->release_hardware())
    {
        INFO(("The capture API has been released."));

        return true;
    }
    else
    {
        NBENE(("Failed to release the capture API."));

        return false;
    }
}

bool capture_api_rgbeasy_s::release_hardware(void)
{
    if (!apicall_succeeded(RGBCloseInput(CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBFree(RGBAPI_HANDLE)))
    {
        return false;
    }

    return true;
}

PIXELFORMAT capture_api_rgbeasy_s::pixel_format_to_rgbeasy_pixel_format(capture_pixel_format_e fmt)
{
    switch (fmt)
    {
        case capture_pixel_format_e::rgb_555: return RGB_PIXELFORMAT_555;
        case capture_pixel_format_e::rgb_565: return RGB_PIXELFORMAT_565;
        case capture_pixel_format_e::rgb_888: return RGB_PIXELFORMAT_888;
        default: k_assert(0, "Unknown pixel format.");
    }
}

bool capture_api_rgbeasy_s::initialize_hardware(void)
{
    INFO(("Initializing the capture hardware."));

    if (!apicall_succeeded(RGBLoad(&RGBAPI_HANDLE)))
    {
        goto fail;
    }

    if (!apicall_succeeded(RGBOpenInput(INPUT_CHANNEL_IDX,      &CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetFrameDropping(CAPTURE_HANDLE,   FRAME_SKIP)) ||
        !apicall_succeeded(RGBSetDMADirect(CAPTURE_HANDLE,       FALSE)) ||
        !apicall_succeeded(RGBSetPixelFormat(CAPTURE_HANDLE,     pixel_format_to_rgbeasy_pixel_format(CAPTURE_PIXEL_FORMAT))) ||
        !apicall_succeeded(RGBUseOutputBuffers(CAPTURE_HANDLE,   FALSE)) ||
        !apicall_succeeded(RGBSetFrameCapturedFn(CAPTURE_HANDLE, rgbeasy_callback__frame_captured, 0)) ||
        !apicall_succeeded(RGBSetModeChangedFn(CAPTURE_HANDLE,   rgbeasy_callback__video_mode_changed, 0)) ||
        !apicall_succeeded(RGBSetInvalidSignalFn(CAPTURE_HANDLE, rgbeasy_callback__invalid_signal, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetErrorFn(CAPTURE_HANDLE,         rgbeasy_callback__error, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetNoSignalFn(CAPTURE_HANDLE,      rgbeasy_callback__no_signal, (ULONG_PTR)&CAPTURE_HANDLE)))
    {
        NBENE(("Failed to initialize the capture hardware."));
        goto fail;
    }

    /// Temp hack. We've only allocated enough room in the input frame buffer to
    /// hold at most the maximum output size.
    {
        const resolution_s maxCaptureRes = this->get_maximum_resolution();

        k_assert((maxCaptureRes.w <= MAX_OUTPUT_WIDTH &&
                  maxCaptureRes.h <= MAX_OUTPUT_HEIGHT),
                 "The capture hardware is not compatible with this version of VCS.");
    }

    return true;

fail:
    return false;
}

bool capture_api_rgbeasy_s::start_capture(void)
{
    INFO(("Starting capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (!apicall_succeeded(RGBStartCapture(CAPTURE_HANDLE)))
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

bool capture_api_rgbeasy_s::stop_capture(void)
{
    INFO(("Stopping capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (CAPTURE_IS_ACTIVE)
    {
        if (!apicall_succeeded(RGBStopCapture(CAPTURE_HANDLE)))
        {
            NBENE(("Failed to stop capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));
            goto fail;
        }
    }
    else
    {
        NBENE(("Was asked to stop the capture even though it hadn't been started."));
        goto fail;
    }

    CAPTURE_IS_ACTIVE = false;

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

void capture_api_rgbeasy_s::update_known_video_mode_params(const resolution_s r,
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
                           this->get_default_color_settings(),
                           this->get_default_video_settings()});

    mode_exists:
    // Update the existing mode with the new parameters.
    if (c != nullptr) KNOWN_MODES[idx].color = *c;
    if (v != nullptr) KNOWN_MODES[idx].video = *v;

    return;
}

bool capture_api_rgbeasy_s::apicall_succeeded(long callReturnValue) const
{
    if (callReturnValue != RGBERROR_NO_ERROR)
    {
        NBENE(("A call to the RGBEasy API returned with error code (0x%x).", callReturnValue));
        return false;
    }

    return true;
}

bool capture_api_rgbeasy_s::device_supports_component_capture(void) const
{
    const int numInputChannels = this->get_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsComponentSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool capture_api_rgbeasy_s::device_supports_composite_capture(void) const
{
    const int numInputChannels = this->get_maximum_input_count();
    long isSupported = 0;

    for (int i = 0; i < numInputChannels; i++)
    {
        if (!apicall_succeeded(RGBInputIsCompositeSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool capture_api_rgbeasy_s::device_supports_deinterlacing(void) const
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsDeinterlaceSupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

bool capture_api_rgbeasy_s::device_supports_svideo(void) const
{
    const int numInputChannels = this->get_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsSVideoSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool capture_api_rgbeasy_s::device_supports_dma(void) const
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsDirectDMASupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

bool capture_api_rgbeasy_s::device_supports_dvi(void) const
{
    const int numInputChannels = this->get_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsDVISupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool capture_api_rgbeasy_s::device_supports_vga(void) const
{
    const int numInputChannels = this->get_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsVGASupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool capture_api_rgbeasy_s::device_supports_yuv(void) const
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsYUVSupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

std::string capture_api_rgbeasy_s::get_device_firmware_version(void) const
{
    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeded(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii)))
    {
        return "Unknown";
    }

    return std::to_string(ii.FirmWare);
}

std::string capture_api_rgbeasy_s::get_device_driver_version(void) const
{
    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeded(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii)))
    {
        return "Unknown";
    }

    return std::string(std::to_string(ii.Driver.Major) + "." +
                       std::to_string(ii.Driver.Minor) + "." +
                       std::to_string(ii.Driver.Micro) + "/" +
                       std::to_string(ii.Driver.Revision));
}

std::string capture_api_rgbeasy_s::get_device_name(void) const
{
    const std::string unknownName = "Unknown capture device";

    CAPTURECARD card = RGB_CAPTURECARD_DGC103;

    if (!apicall_succeeded(RGBGetCaptureCard(&card)))
    {
        return unknownName;
    }

    switch (card)
    {
        case RGB_CAPTURECARD_DGC103: return "Datapath DGC103 Series";
        case RGB_CAPTURECARD_DGC133: return "Datapath DGC133 Series";
        default:                     return unknownName;
    }
}

std::string capture_api_rgbeasy_s::get_api_name(void) const
{
    return "RGBEasy";
}

int capture_api_rgbeasy_s::get_maximum_input_count(void) const
{
    unsigned long numInputs = 0;

    if (!apicall_succeeded(RGBGetNumberOfInputs(&numInputs)))
    {
        return 0;
    }

    return numInputs;
}

capture_color_settings_s capture_api_rgbeasy_s::get_color_settings(void) const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeded(RGBGetBrightness(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrast(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalance(CAPTURE_HANDLE, &p.redBrightness,
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

capture_color_settings_s capture_api_rgbeasy_s::get_default_color_settings(void) const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeded(RGBGetBrightnessDefault(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastDefault(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceDefault(CAPTURE_HANDLE, &p.redBrightness,
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

capture_color_settings_s capture_api_rgbeasy_s::get_minimum_color_settings(void) const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeded(RGBGetBrightnessMinimum(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMinimum(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMinimum(CAPTURE_HANDLE, &p.redBrightness,
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

capture_color_settings_s capture_api_rgbeasy_s::get_maximum_color_settings(void) const
{
    capture_color_settings_s p = {0};

    if (!apicall_succeeded(RGBGetBrightnessMaximum(CAPTURE_HANDLE, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMaximum(CAPTURE_HANDLE, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMaximum(CAPTURE_HANDLE, &p.redBrightness,
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

capture_video_settings_s capture_api_rgbeasy_s::get_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhase(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevel(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPosition(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPosition(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScale(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_default_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseDefault(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelDefault(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionDefault(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionDefault(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleDefault(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_minimum_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMinimum(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMinimum(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMinimum(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMinimum(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMinimum(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_maximum_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMaximum(CAPTURE_HANDLE, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMaximum(CAPTURE_HANDLE, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMaximum(CAPTURE_HANDLE, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMaximum(CAPTURE_HANDLE, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMaximum(CAPTURE_HANDLE, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

resolution_s capture_api_rgbeasy_s::get_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidth(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeight(CAPTURE_HANDLE, &r.h)))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    r.bpp = CAPTURE_OUTPUT_COLOR_DEPTH;

    return r;
}

resolution_s capture_api_rgbeasy_s::get_minimum_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidthMinimum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMinimum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }

    // NOTE: It's assumed that 16 bits is the minimum capture color depth.
    r.bpp = 16;

    return r;
}

resolution_s capture_api_rgbeasy_s::get_maximum_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidthMaximum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMaximum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }

    // NOTE: It's assumed that 32 bits is the maximum capture color depth.
    r.bpp = 32;

    return r;
}

const captured_frame_s& capture_api_rgbeasy_s::reserve_frame_buffer(void)
{
    RGBEASY_CALLBACK_MUTEX.lock();

    return FRAME_BUFFER;
}

void capture_api_rgbeasy_s::unreserve_frame_buffer(void)
{
    SKIP_NEXT_NUM_FRAMES -= bool(SKIP_NEXT_NUM_FRAMES);

    FRAME_BUFFER.processed = true;

    RGBEASY_CALLBACK_MUTEX.unlock();

    return;
}

capture_event_e capture_api_rgbeasy_s::pop_capture_event_queue(void)
{
    std::lock_guard<std::mutex> lock(RGBEASY_CALLBACK_MUTEX);

    if (pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::no_signal))
    {
        return capture_event_e::no_signal;
    }
    else if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    return capture_event_e::sleep;
}

capture_signal_s capture_api_rgbeasy_s::get_signal_info(void) const
{
    if (kc_capture_api().get_no_signal())
    {
        NBENE(("Tried to query the capture signal while no signal was being received."));
        return {0};
    }

    capture_signal_s s = {0};

    RGBMODEINFO mi = {0};
    mi.Size = sizeof(mi);

    if (apicall_succeeded(RGBGetModeInfo(CAPTURE_HANDLE, &mi)))
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

    s.r = this->get_resolution();

    return s;
}

int capture_api_rgbeasy_s::get_frame_rate(void) const
{
    unsigned long rate = 0;

    if (!apicall_succeeded(RGBGetFrameRate(CAPTURE_HANDLE, &rate)))
    {
        return 0;
    }

    return rate;
}

/////////////////////////////

void capture_api_rgbeasy_s::set_mode_params(const std::vector<video_mode_params_s> &modeParams)
{
    KNOWN_MODES = modeParams;

    return;
}

bool capture_api_rgbeasy_s::set_mode_parameters_for_resolution(const resolution_s r)
{
    //INFO(("Applying mode parameters for %u x %u.", r.w, r.h));

    video_mode_params_s p = kc_capture_api().get_mode_params_for_resolution(r);

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

    (void)p;

    return true;
}

void capture_api_rgbeasy_s::set_color_settings(const capture_color_settings_s c)
{
    if (kc_capture_api().get_no_signal())
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

    update_known_video_mode_params(this->get_resolution(), &c, nullptr);

    return;
}

void capture_api_rgbeasy_s::set_video_settings(const capture_video_settings_s v)
{
    if (kc_capture_api().get_no_signal())
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

    update_known_video_mode_params(this->get_resolution(), nullptr, &v);

    return;
}

bool capture_api_rgbeasy_s::adjust_video_horizontal_offset(const int delta)
{
    if (!delta)
    {
        return true;
    }

    if (!RECEIVING_A_SIGNAL)
    {
        return false;
    }

    const long newPos = (this->get_video_settings().horizontalPosition + delta);
    if (newPos < this->get_minimum_video_settings().horizontalPosition ||
        newPos > this->get_maximum_video_settings().horizontalPosition)
    {
        return false;
    }

    if (apicall_succeeded(RGBSetHorPosition(CAPTURE_HANDLE, newPos)))
    {
        // Assume that this was a user-requested change, and as such that it
        // should affect the user's custom mode parameter settings.
        const auto currentVideoParams = this->get_video_settings();
        update_known_video_mode_params(this->get_resolution(), nullptr, &currentVideoParams);

        kd_update_video_mode_params();
    }

    return true;
}

bool capture_api_rgbeasy_s::adjust_video_vertical_offset(const int delta)
{
    if (!delta) return true;
    if (!RECEIVING_A_SIGNAL) return false;

    const long newPos = (this->get_video_settings().verticalPosition + delta);
    if (newPos < std::max(2, (int)this->get_minimum_video_settings().verticalPosition) ||
        newPos > this->get_maximum_video_settings().verticalPosition)
    {
        // ^ Testing for < 2 along with < MIN_VIDEO_PARAMS.verPos, since on my
        // VisionRGB-PRO2, MIN_VIDEO_PARAMS.verPos gives a value less than 2,
        // but setting any such value corrupts the captured image.
        return false;
    }

    if (apicall_succeeded(RGBSetVerPosition(CAPTURE_HANDLE, newPos)))
    {
        // Assume that this was a user-requested change, and as such that it
        // should affect the user's custom mode parameter settings.
        const auto currentVideoParams = this->get_video_settings();
        update_known_video_mode_params(this->get_resolution(), nullptr, &currentVideoParams);

        kd_update_video_mode_params();
    }

    return true;
}

bool capture_api_rgbeasy_s::set_input_channel(const unsigned channel)
{
    const int numInputChannels = this->get_maximum_input_count();

    if (numInputChannels < 0)
    {
        NBENE(("Encountered an unexpected error while setting the input channel. Aborting the action."));

        goto fail;
    }

    if (channel >= (unsigned)numInputChannels)
    {
        INFO(("Was asked to set an input channel that is out of bounds. Ignoring the request."));

        goto fail;
    }

    if (apicall_succeeded(RGBSetInput(CAPTURE_HANDLE, channel)))
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

bool capture_api_rgbeasy_s::set_input_color_depth(const unsigned bpp)
{
    const capture_pixel_format_e previousFormat = CAPTURE_PIXEL_FORMAT;
    const uint previousColorDepth = CAPTURE_OUTPUT_COLOR_DEPTH;

    switch (bpp)
    {
        case 24: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888; CAPTURE_OUTPUT_COLOR_DEPTH = 32; break;
        case 16: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_565; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
        case 15: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_555; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
        default: k_assert(0, "Was asked to set an unknown pixel format."); break;
    }

    if (!apicall_succeeded(RGBSetPixelFormat(CAPTURE_HANDLE, pixel_format_to_rgbeasy_pixel_format(CAPTURE_PIXEL_FORMAT))))
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

bool capture_api_rgbeasy_s::set_frame_dropping(const unsigned drop)
{
    // Sanity check.
    k_assert(drop < 100, "Odd frame drop number.");

    if (apicall_succeeded(RGBSetFrameDropping(CAPTURE_HANDLE, drop)))
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

void capture_api_rgbeasy_s::apply_new_capture_resolution(void)
{
    resolution_s resolution = this->get_resolution();
    const resolution_s aliasedRes = ka_aliased(resolution);

    // If the current resolution has an alias, switch to that.
    if ((resolution.w != aliasedRes.w) ||
        (resolution.h != aliasedRes.h))
    {
        if (!kc_capture_api().set_resolution(aliasedRes))
        {
            NBENE(("Failed to apply an alias."));
        }
        else resolution = aliasedRes;
    }

    kc_capture_api().set_mode_parameters_for_resolution(resolution);

    RECEIVED_NEW_VIDEO_MODE = false;

    INFO(("New input mode: %u x %u @ %u Hz.", resolution.w, resolution.h, this->get_signal_info().refreshRate));

    return;
}

void capture_api_rgbeasy_s::reset_missed_frames_count(void)
{
    NUM_NEW_FRAME_EVENTS_SKIPPED = 0;

    return;
}

bool capture_api_rgbeasy_s::set_resolution(const resolution_s &r)
{
    if (!CAPTURE_IS_ACTIVE)
    {
        INFO(("Was asked to set the capture resolution while capture was inactive. Ignoring this request."));
        return false;
    }

    unsigned long wd = 0, hd = 0;

    const auto currentInputRes = kc_capture_api().get_resolution();
    if (r.w == currentInputRes.w &&
        r.h == currentInputRes.h)
    {
        DEBUG(("Was asked to force a capture resolution that had already been set. Ignoring the request."));
        goto fail;
    }

    // Test whether the capture hardware can handle the given resolution.
    if (!apicall_succeeded(RGBTestCaptureWidth(CAPTURE_HANDLE, r.w)))
    {
        NBENE(("Failed to force the new input resolution (%u x %u). The capture hardware says the width "
               "is illegal.", r.w, r.h));
        goto fail;
    }

    // Set the new resolution.
    if (!apicall_succeeded(RGBSetCaptureWidth(CAPTURE_HANDLE, (unsigned long)r.w)) ||
        !apicall_succeeded(RGBSetCaptureHeight(CAPTURE_HANDLE, (unsigned long)r.h)) ||
        !apicall_succeeded(RGBSetOutputSize(CAPTURE_HANDLE, (unsigned long)r.w, (unsigned long)r.h)))
    {
        NBENE(("The capture hardware could not properly initialize the new input resolution (%u x %u).",
                r.w, r.h));
        goto fail;
    }

    /// Temp hack to test if the new resolution is valid.
    RGBGetOutputSize(CAPTURE_HANDLE, &wd, &hd);
    if (wd != r.w ||
        hd != r.h)
    {
        NBENE(("The capture hardware failed to set the desired resolution."));
        goto fail;
    }

    // Avoid garbage in the frame buffer while the resolution changes.
    SKIP_NEXT_NUM_FRAMES += 2;

    return true;

    fail:
    return false;
}

uint capture_api_rgbeasy_s::get_num_missed_frames(void)
{
    return NUM_NEW_FRAME_EVENTS_SKIPPED;
}

uint capture_api_rgbeasy_s::get_input_channel_idx(void)
{
    return INPUT_CHANNEL_IDX;
}

uint capture_api_rgbeasy_s::get_input_color_depth(void)
{
    switch (CAPTURE_PIXEL_FORMAT)
    {
        case capture_pixel_format_e::rgb_888: return 24;
        case capture_pixel_format_e::rgb_565: return 16;
        case capture_pixel_format_e::rgb_555: return 15;
        default: k_assert(0, "Unknown capture pixel format."); return 0;
    }
}

bool capture_api_rgbeasy_s::get_are_frames_being_dropped(void)
{
    return bool(NUM_NEW_FRAME_EVENTS_SKIPPED > 0);
}

bool capture_api_rgbeasy_s::get_is_capture_active(void)
{
    return CAPTURE_IS_ACTIVE;
}

bool capture_api_rgbeasy_s::get_should_current_frame_be_skipped(void)
{
    return bool(SKIP_NEXT_NUM_FRAMES > 0);
}

bool capture_api_rgbeasy_s::get_is_invalid_signal(void)
{
    return IS_SIGNAL_INVALID;
}

bool capture_api_rgbeasy_s::get_no_signal(void)
{
    return !RECEIVING_A_SIGNAL;
}

capture_pixel_format_e capture_api_rgbeasy_s::get_pixel_format(void)
{
    return CAPTURE_PIXEL_FORMAT;
}

const std::vector<video_mode_params_s>& capture_api_rgbeasy_s::get_mode_params(void)
{
    return KNOWN_MODES;
}

video_mode_params_s capture_api_rgbeasy_s::get_mode_params_for_resolution(const resolution_s r)
{
    for (const auto &m: KNOWN_MODES)
    {
        if (m.r.w == r.w &&
            m.r.h == r.h)
        {
            return m;
        }
    }

    //INFO(("Unknown video mode; returning default parameters."));
    
    return {r,
            this->get_default_color_settings(),
            this->get_default_video_settings()};
}
