#include <atomic>
#include <cmath>
#include "common/globals.h"
#include "common/propagate.h"
#include "capture/capture_api_rgbeasy.h"
#include "capture/capture.h"
#include "capture/alias.h"

// Frames sent by the capture hardware will be stored here for processing.
static captured_frame_s FRAME_BUFFER;

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and so which we had to skip.
static std::atomic<unsigned int> NUM_NEW_FRAME_EVENTS_SKIPPED(0);

// Whether the capture hardware is receiving a signal from its input.
static bool RECEIVING_A_SIGNAL = true;

// If the current signal we're receiving is invalid.
static bool IS_SIGNAL_INVALID = false;

// The maximum image depth that the capturer can handle.
static const unsigned MAX_BIT_DEPTH = 32;

// Marks the given capture event as having occurred.
void capture_api_rgbeasy_s::push_capture_event(capture_event_e event)
{
    this->rgbeasyCaptureEventFlags[static_cast<int>(event)] = true;

    return;
}

// Removes the given capture event from the 'queue', and returns its value.
bool capture_api_rgbeasy_s::pop_capture_event(capture_event_e event)
{
    const bool eventOccurred = this->rgbeasyCaptureEventFlags[static_cast<int>(event)];
    this->rgbeasyCaptureEventFlags[static_cast<int>(event)] = false;

    return eventOccurred;
}

// Callback functions for the RGBEasy API, through which the API communicates
// with VCS. RGBEasy isn't supported on platforms other than Windows, hence the
// #if - on other platforms, we load in empty placeholder functions (elsewhere
// in the code).
namespace rgbeasy_callbacks_n
{
#if _WIN32
    // Called by the capture hardware when a new frame has been captured. The
    // captured RGBA data is in frameData.
    void RGBCBKAPI frame_captured(HWND, HRGB, LPBITMAPINFOHEADER frameInfo, void *frameData, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        // If the hardware is sending us a new frame while we're still unfinished
        // with processing the previous frame. In that case, we'll need to skip
        // this new frame.
        if (!thisPtr->try_to_lock_rgbeasy_mutex())
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

        thisPtr->push_capture_event(capture_event_e::new_frame);

        done:
        thisPtr->unlock_rgbeasy_mutex();
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI video_mode_changed(HWND, HRGB, PRGBMODECHANGEDINFO, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        thisPtr->lock_rgbeasy_mutex();

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        thisPtr->push_capture_event(capture_event_e::new_video_mode);

        IS_SIGNAL_INVALID = false;
        RECEIVING_A_SIGNAL = true;

        done:
        thisPtr->unlock_rgbeasy_mutex();
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI invalid_signal(HWND, HRGB, unsigned long horClock, unsigned long verClock, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        thisPtr->lock_rgbeasy_mutex();

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // Let the card apply its own no signal handler as well, just in case.
        RGBInvalidSignal(thisPtr->rgbeasy_capture_handle(), horClock, verClock);

        thisPtr->push_capture_event(capture_event_e::invalid_signal);

        IS_SIGNAL_INVALID = true;
        RECEIVING_A_SIGNAL = false;

        done:
        thisPtr->unlock_rgbeasy_mutex();
        return;
    }

    // Called by the capture hardware when no input signal is present.
    void RGBCBKAPI no_signal(HWND, HRGB, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        thisPtr->lock_rgbeasy_mutex();

        // Let the card apply its own 'no signal' handler as well, just in case.
        RGBNoSignal(thisPtr->rgbeasy_capture_handle());

        thisPtr->push_capture_event(capture_event_e::no_signal);

        RECEIVING_A_SIGNAL = false;

        thisPtr->unlock_rgbeasy_mutex();
        return;
    }

    void RGBCBKAPI error(HWND, HRGB, unsigned long, ULONG_PTR _thisPtr, unsigned long*)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        thisPtr->lock_rgbeasy_mutex();

        thisPtr->push_capture_event(capture_event_e::unrecoverable_error);

        RECEIVING_A_SIGNAL = false;

        thisPtr->unlock_rgbeasy_mutex();
        return;
    }
#endif
}

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
    if (!apicall_succeeded(RGBCloseInput(this->captureHandle)) ||
        !apicall_succeeded(RGBFree(this->rgbAPIHandle)))
    {
        return false;
    }

    return true;
}

PIXELFORMAT capture_api_rgbeasy_s::pixel_format_to_rgbeasy_pixel_format(capturePixelFormat_e fmt)
{
    switch (fmt)
    {
        case capturePixelFormat_e::rgb_555: return RGB_PIXELFORMAT_555;
        case capturePixelFormat_e::rgb_565: return RGB_PIXELFORMAT_565;
        case capturePixelFormat_e::rgb_888: return RGB_PIXELFORMAT_888;
        default: k_assert(0, "Unknown pixel format.");
    }
}

bool capture_api_rgbeasy_s::initialize_hardware(void)
{
    INFO(("Initializing the capture hardware."));

    if (!apicall_succeeded(RGBLoad(&this->rgbAPIHandle)))
    {
        goto fail;
    }

    if (!apicall_succeeded(RGBOpenInput(INPUT_CHANNEL_IDX,      &this->captureHandle)) ||
        !apicall_succeeded(RGBSetFrameDropping(this->captureHandle,   FRAME_SKIP)) ||
        !apicall_succeeded(RGBSetDMADirect(this->captureHandle,       FALSE)) ||
        !apicall_succeeded(RGBSetPixelFormat(this->captureHandle,     pixel_format_to_rgbeasy_pixel_format(this->capturePixelFormat))) ||
        !apicall_succeeded(RGBUseOutputBuffers(this->captureHandle,   FALSE)) ||
        !apicall_succeeded(RGBSetFrameCapturedFn(this->captureHandle, rgbeasy_callbacks_n::frame_captured,     (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetModeChangedFn(this->captureHandle,   rgbeasy_callbacks_n::video_mode_changed, (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetInvalidSignalFn(this->captureHandle, rgbeasy_callbacks_n::invalid_signal,     (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetErrorFn(this->captureHandle,         rgbeasy_callbacks_n::error,              (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetNoSignalFn(this->captureHandle,      rgbeasy_callbacks_n::no_signal,          (ULONG_PTR)this)))
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

    if (!apicall_succeeded(RGBStartCapture(this->captureHandle)))
    {
        NBENE(("Failed to start capture on input channel %u.", (INPUT_CHANNEL_IDX + 1)));
        goto fail;
    }
    else
    {
        this->captureIsActive = true;
    }

    return true;

    fail:
    return false;
}

bool capture_api_rgbeasy_s::stop_capture(void)
{
    INFO(("Stopping capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (this->captureIsActive)
    {
        if (!apicall_succeeded(RGBStopCapture(this->captureHandle)))
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

    this->captureIsActive = false;

    INFO(("Restoring default callback handlers."));
    RGBSetFrameCapturedFn(this->captureHandle, NULL, 0);
    RGBSetModeChangedFn(this->captureHandle, NULL, 0);
    RGBSetInvalidSignalFn(this->captureHandle, NULL, 0);
    RGBSetNoSignalFn(this->captureHandle, NULL, 0);
    RGBSetErrorFn(this->captureHandle, NULL, 0);

    return true;

    fail:
    return false;
}

void capture_api_rgbeasy_s::update_known_video_mode_params(const resolution_s r,
                                                           const capture_color_settings_s *const c,
                                                           const capture_video_settings_s *const v)
{
    uint idx;
    for (idx = 0; idx < this->knownVideoModes.size(); idx++)
    {
        if (this->knownVideoModes[idx].r.w == r.w &&
            this->knownVideoModes[idx].r.h == r.h)
        {
            goto mode_exists;
        }
    }

    // If the mode doesn't already exist, add it.
    this->knownVideoModes.push_back({r,
                           this->get_default_color_settings(),
                           this->get_default_video_settings()});

    mode_exists:
    // Update the existing mode with the new parameters.
    if (c != nullptr) this->knownVideoModes[idx].color = *c;
    if (v != nullptr) this->knownVideoModes[idx].video = *v;

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
    const int numInputChannels = this->get_device_max_input_count();

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
    const int numInputChannels = this->get_device_max_input_count();
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
    const int numInputChannels = this->get_device_max_input_count();

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
    const int numInputChannels = this->get_device_max_input_count();

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
    const int numInputChannels = this->get_device_max_input_count();

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

void capture_api_rgbeasy_s::lock_rgbeasy_mutex(void)
{
    this->rgbeasyCallbackMutex.lock();

    return;
}

bool capture_api_rgbeasy_s::try_to_lock_rgbeasy_mutex(void)
{
    return this->rgbeasyCallbackMutex.try_lock();
}

void capture_api_rgbeasy_s::unlock_rgbeasy_mutex(void)
{
    this->rgbeasyCallbackMutex.unlock();

    return;
}

HRGB capture_api_rgbeasy_s::rgbeasy_capture_handle(void)
{
    return this->captureHandle;
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

int capture_api_rgbeasy_s::get_device_max_input_count(void) const
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

    if (!apicall_succeeded(RGBGetBrightness(this->captureHandle, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrast(this->captureHandle, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalance(this->captureHandle, &p.redBrightness,
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

    if (!apicall_succeeded(RGBGetBrightnessDefault(this->captureHandle, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastDefault(this->captureHandle, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceDefault(this->captureHandle, &p.redBrightness,
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

    if (!apicall_succeeded(RGBGetBrightnessMinimum(this->captureHandle, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMinimum(this->captureHandle, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMinimum(this->captureHandle, &p.redBrightness,
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

    if (!apicall_succeeded(RGBGetBrightnessMaximum(this->captureHandle, &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMaximum(this->captureHandle, &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMaximum(this->captureHandle, &p.redBrightness,
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

    if (!apicall_succeeded(RGBGetPhase(this->captureHandle, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevel(this->captureHandle, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPosition(this->captureHandle, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPosition(this->captureHandle, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScale(this->captureHandle, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_default_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseDefault(this->captureHandle, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelDefault(this->captureHandle, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionDefault(this->captureHandle, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionDefault(this->captureHandle, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleDefault(this->captureHandle, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_minimum_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMinimum(this->captureHandle, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMinimum(this->captureHandle, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMinimum(this->captureHandle, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMinimum(this->captureHandle, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMinimum(this->captureHandle, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

capture_video_settings_s capture_api_rgbeasy_s::get_maximum_video_settings(void) const
{
    capture_video_settings_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMaximum(this->captureHandle, &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMaximum(this->captureHandle, &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMaximum(this->captureHandle, &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMaximum(this->captureHandle, &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMaximum(this->captureHandle, &p.horizontalScale)))
    {
        return {0};
    }

    return p;
}

resolution_s capture_api_rgbeasy_s::get_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidth(this->captureHandle, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeight(this->captureHandle, &r.h)))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    r.bpp = (unsigned)this->capturePixelFormat;

    return r;
}

resolution_s capture_api_rgbeasy_s::get_minimum_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidthMinimum(this->captureHandle, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMinimum(this->captureHandle, &r.h)))
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

    if (!apicall_succeeded(RGBGetCaptureWidthMaximum(this->captureHandle, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMaximum(this->captureHandle, &r.h)))
    {
        return {0};
    }

    // NOTE: It's assumed that 32 bits is the maximum capture color depth.
    r.bpp = 32;

    return r;
}

const captured_frame_s& capture_api_rgbeasy_s::reserve_frame_buffer(void)
{
    rgbeasyCallbackMutex.lock();

    return FRAME_BUFFER;
}

void capture_api_rgbeasy_s::unreserve_frame_buffer(void)
{
    this->skipNextNumFrames -= bool(this->skipNextNumFrames);

    FRAME_BUFFER.processed = true;

    rgbeasyCallbackMutex.unlock();

    return;
}

capture_event_e capture_api_rgbeasy_s::pop_capture_event_queue(void)
{
    std::lock_guard<std::mutex> lock(rgbeasyCallbackMutex);

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

    if (apicall_succeeded(RGBGetModeInfo(this->captureHandle, &mi)))
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

void capture_api_rgbeasy_s::set_mode_params(const std::vector<video_mode_params_s> &modeParams)
{
    this->knownVideoModes = modeParams;

    return;
}

bool capture_api_rgbeasy_s::set_mode_parameters_for_resolution(const resolution_s r)
{
    //INFO(("Applying mode parameters for %u x %u.", r.w, r.h));

    video_mode_params_s p = kc_capture_api().get_mode_params_for_resolution(r);

    // Apply the set of mode parameters for the current input resolution.
    /// TODO. Add error-checking.
    RGBSetPhase(this->captureHandle,         p.video.phase);
    RGBSetBlackLevel(this->captureHandle,    p.video.blackLevel);
    RGBSetHorScale(this->captureHandle,      p.video.horizontalScale);
    RGBSetHorPosition(this->captureHandle,   p.video.horizontalPosition);
    RGBSetVerPosition(this->captureHandle,   p.video.verticalPosition);
    RGBSetBrightness(this->captureHandle,    p.color.overallBrightness);
    RGBSetContrast(this->captureHandle,      p.color.overallContrast);
    RGBSetColourBalance(this->captureHandle, p.color.redBrightness,
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

    RGBSetBrightness(this->captureHandle, c.overallBrightness);
    RGBSetContrast(this->captureHandle, c.overallContrast);
    RGBSetColourBalance(this->captureHandle, c.redBrightness,
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

    RGBSetPhase(this->captureHandle, v.phase);
    RGBSetBlackLevel(this->captureHandle, v.blackLevel);
    RGBSetHorPosition(this->captureHandle, v.horizontalPosition);
    RGBSetHorScale(this->captureHandle, v.horizontalScale);
    RGBSetVerPosition(this->captureHandle, v.verticalPosition);

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

    if (apicall_succeeded(RGBSetHorPosition(this->captureHandle, newPos)))
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

    if (apicall_succeeded(RGBSetVerPosition(this->captureHandle, newPos)))
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
    const int numInputChannels = this->get_device_max_input_count();

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

    if (apicall_succeeded(RGBSetInput(this->captureHandle, channel)))
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
    const capturePixelFormat_e previousFormat = this->capturePixelFormat;

    switch (bpp)
    {
        case 24: this->capturePixelFormat = capturePixelFormat_e::rgb_888; break;
        case 16: this->capturePixelFormat = capturePixelFormat_e::rgb_565; break;
        case 15: this->capturePixelFormat = capturePixelFormat_e::rgb_555; break;
        default: k_assert(0, "Was asked to set an unknown pixel format."); break;
    }

    if (!apicall_succeeded(RGBSetPixelFormat(this->captureHandle, pixel_format_to_rgbeasy_pixel_format(this->capturePixelFormat))))
    {
        this->capturePixelFormat = previousFormat;

        goto fail;
    }

    // Ignore the next frame to avoid displaying some visual corruption from
    // switching the bit depth.
    this->skipNextNumFrames += 1;

    return true;

    fail:
    return false;
}

bool capture_api_rgbeasy_s::set_frame_dropping(const unsigned drop)
{
    // Sanity check.
    k_assert(drop < 100, "Odd frame drop number.");

    if (apicall_succeeded(RGBSetFrameDropping(this->captureHandle, drop)))
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
    if (!this->captureIsActive)
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
    if (!apicall_succeeded(RGBTestCaptureWidth(this->captureHandle, r.w)))
    {
        NBENE(("Failed to force the new input resolution (%u x %u). The capture hardware says the width "
               "is illegal.", r.w, r.h));
        goto fail;
    }

    // Set the new resolution.
    if (!apicall_succeeded(RGBSetCaptureWidth(this->captureHandle, (unsigned long)r.w)) ||
        !apicall_succeeded(RGBSetCaptureHeight(this->captureHandle, (unsigned long)r.h)) ||
        !apicall_succeeded(RGBSetOutputSize(this->captureHandle, (unsigned long)r.w, (unsigned long)r.h)))
    {
        NBENE(("The capture hardware could not properly initialize the new input resolution (%u x %u).",
                r.w, r.h));
        goto fail;
    }

    /// Temp hack to test if the new resolution is valid.
    RGBGetOutputSize(this->captureHandle, &wd, &hd);
    if (wd != r.w ||
        hd != r.h)
    {
        NBENE(("The capture hardware failed to set the desired resolution."));
        goto fail;
    }

    // Avoid garbage in the frame buffer while the resolution changes.
    this->skipNextNumFrames += 2;

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
    switch (this->capturePixelFormat)
    {
        case capturePixelFormat_e::rgb_888: return 24;
        case capturePixelFormat_e::rgb_565: return 16;
        case capturePixelFormat_e::rgb_555: return 15;
        default: k_assert(0, "Unknown capture pixel format."); return 0;
    }
}

bool capture_api_rgbeasy_s::get_are_frames_being_dropped(void)
{
    return bool(NUM_NEW_FRAME_EVENTS_SKIPPED > 0);
}

bool capture_api_rgbeasy_s::get_is_capture_active(void)
{
    return this->captureIsActive;
}

bool capture_api_rgbeasy_s::get_should_current_frame_be_skipped(void)
{
    return bool(this->skipNextNumFrames > 0);
}

bool capture_api_rgbeasy_s::get_is_invalid_signal(void)
{
    return IS_SIGNAL_INVALID;
}

bool capture_api_rgbeasy_s::get_no_signal(void)
{
    return !RECEIVING_A_SIGNAL;
}

capturePixelFormat_e capture_api_rgbeasy_s::get_pixel_format(void)
{
    return this->capturePixelFormat;
}

const std::vector<video_mode_params_s>& capture_api_rgbeasy_s::get_mode_params(void)
{
    return this->knownVideoModes;
}

video_mode_params_s capture_api_rgbeasy_s::get_mode_params_for_resolution(const resolution_s r)
{
    for (const auto &m: this->knownVideoModes)
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