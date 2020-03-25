#include <atomic>
#include <cmath>
#include "common/globals.h"
#include "common/propagate/propagate.h"
#include "capture/capture_api_rgbeasy.h"
#include "capture/video_parameters.h"
#include "capture/capture.h"
#include "capture/alias.h"

static std::atomic<unsigned int> CNT_FRAMES_PROCESSED(0);
static std::atomic<unsigned int> CNT_FRAMES_CAPTURED(0);

// The pixel format in which the capture device sends captured frames.
static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

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
        if (CNT_FRAMES_CAPTURED != CNT_FRAMES_PROCESSED)
        {
            NUM_NEW_FRAME_EVENTS_SKIPPED++;
            return;
        }

        std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

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
        FRAME_BUFFER.pixelFormat = CAPTURE_PIXEL_FORMAT;

        // Copy the frame's data into our local buffer so we can work on it.
        memcpy(FRAME_BUFFER.pixels.ptr(), (u8*)frameData,
               FRAME_BUFFER.pixels.up_to(FRAME_BUFFER.r.w * FRAME_BUFFER.r.h * (FRAME_BUFFER.r.bpp / 8)));

        thisPtr->push_capture_event(capture_event_e::new_frame);

        done:
        CNT_FRAMES_CAPTURED++;
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI video_mode_changed(HWND, HRGB, PRGBMODECHANGEDINFO, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        thisPtr->push_capture_event(capture_event_e::new_video_mode);

        IS_SIGNAL_INVALID = false;
        RECEIVING_A_SIGNAL = true;

        done:
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI invalid_signal(HWND, HRGB, unsigned long horClock, unsigned long verClock, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

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
        return;
    }

    // Called by the capture hardware when the input signal is lost.
    void RGBCBKAPI lost_signal(HWND, HRGB, ULONG_PTR _thisPtr)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

        // Let the card apply its own 'no signal' handler as well, just in case.
        RGBNoSignal(thisPtr->rgbeasy_capture_handle());

        thisPtr->push_capture_event(capture_event_e::signal_lost);

        RECEIVING_A_SIGNAL = false;

        return;
    }

    void RGBCBKAPI error(HWND, HRGB, unsigned long, ULONG_PTR _thisPtr, unsigned long*)
    {
        const auto thisPtr = reinterpret_cast<capture_api_rgbeasy_s*>(_thisPtr);

        std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

        thisPtr->push_capture_event(capture_event_e::unrecoverable_error);

        RECEIVING_A_SIGNAL = false;

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
    DEBUG(("Releasing the capture API."));

    FRAME_BUFFER.pixels.release_memory();

    if (this->stop_capture() &&
        this->release_hardware())
    {
        DEBUG(("The capture API has been released."));

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

    if (!apicall_succeeded(RGBLoad(&this->rgbAPIHandle)))
    {
        goto fail;
    }

    if (!apicall_succeeded(RGBOpenInput(INPUT_CHANNEL_IDX,      &this->captureHandle)) ||
        !apicall_succeeded(RGBSetFrameDropping(this->captureHandle,   FRAME_SKIP)) ||
        !apicall_succeeded(RGBSetDMADirect(this->captureHandle,       FALSE)) ||
        !apicall_succeeded(RGBSetPixelFormat(this->captureHandle,     pixel_format_to_rgbeasy_pixel_format(CAPTURE_PIXEL_FORMAT))) ||
        !apicall_succeeded(RGBUseOutputBuffers(this->captureHandle,   FALSE)) ||
        !apicall_succeeded(RGBSetFrameCapturedFn(this->captureHandle, rgbeasy_callbacks_n::frame_captured,     (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetModeChangedFn(this->captureHandle,   rgbeasy_callbacks_n::video_mode_changed, (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetInvalidSignalFn(this->captureHandle, rgbeasy_callbacks_n::invalid_signal,     (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetErrorFn(this->captureHandle,         rgbeasy_callbacks_n::error,              (ULONG_PTR)this)) ||
        !apicall_succeeded(RGBSetNoSignalFn(this->captureHandle,      rgbeasy_callbacks_n::lost_signal,        (ULONG_PTR)this)))
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

    DEBUG(("Restoring default callback handlers."));
    RGBSetFrameCapturedFn(this->captureHandle, NULL, 0);
    RGBSetModeChangedFn(this->captureHandle, NULL, 0);
    RGBSetInvalidSignalFn(this->captureHandle, NULL, 0);
    RGBSetNoSignalFn(this->captureHandle, NULL, 0);
    RGBSetErrorFn(this->captureHandle, NULL, 0);

    return true;

    fail:
    return false;
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

video_signal_parameters_s capture_api_rgbeasy_s::get_video_signal_parameters(void) const
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhase(this->captureHandle,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevel(this->captureHandle,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPosition(this->captureHandle,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPosition(this->captureHandle,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScale(this->captureHandle,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightness(this->captureHandle,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrast(this->captureHandle,      &p.overallContrast)) ||
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

video_signal_parameters_s capture_api_rgbeasy_s::get_default_video_signal_parameters(void) const
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseDefault(this->captureHandle,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelDefault(this->captureHandle,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionDefault(this->captureHandle,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionDefault(this->captureHandle,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleDefault(this->captureHandle,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessDefault(this->captureHandle,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastDefault(this->captureHandle,      &p.overallContrast)) ||
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

video_signal_parameters_s capture_api_rgbeasy_s::get_minimum_video_signal_parameters(void) const
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMinimum(this->captureHandle,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMinimum(this->captureHandle,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMinimum(this->captureHandle,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMinimum(this->captureHandle,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMinimum(this->captureHandle,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessMinimum(this->captureHandle,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMinimum(this->captureHandle,      &p.overallContrast)) ||
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

video_signal_parameters_s capture_api_rgbeasy_s::get_maximum_video_signal_parameters(void) const
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMaximum(this->captureHandle,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMaximum(this->captureHandle,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMaximum(this->captureHandle,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMaximum(this->captureHandle,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMaximum(this->captureHandle,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessMaximum(this->captureHandle,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMaximum(this->captureHandle,      &p.overallContrast)) ||
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

resolution_s capture_api_rgbeasy_s::get_resolution(void) const
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidth(this->captureHandle, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeight(this->captureHandle, &r.h)))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    r.bpp = this->get_color_depth();

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

const captured_frame_s& capture_api_rgbeasy_s::get_frame_buffer(void) const
{
    return FRAME_BUFFER;
}

void capture_api_rgbeasy_s::mark_frame_buffer_as_processed(void)
{
    CNT_FRAMES_PROCESSED = CNT_FRAMES_CAPTURED.load();

    FRAME_BUFFER.processed = true;

    return;
}

capture_event_e capture_api_rgbeasy_s::pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::signal_lost))
    {
        return capture_event_e::signal_lost;
    }
    else if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    // If there were no events we should notify the caller about.
    return (RECEIVING_A_SIGNAL? capture_event_e::none : capture_event_e::sleep);
}

unsigned capture_api_rgbeasy_s::get_refresh_rate(void) const
{
    if (this->has_no_signal())
    {
        NBENE(("Tried to query the capture signal while no signal was being received."));
        return {0};
    }

    RGBMODEINFO mi = {0};
    mi.Size = sizeof(mi);

    if (apicall_succeeded(RGBGetModeInfo(this->captureHandle, &mi)))
    {
        return round(mi.RefreshRate / 1000.0);
    }
    else
    {
        return 0;
    }
}

bool capture_api_rgbeasy_s::assign_video_signal_params_for_resolution(const resolution_s r)
{
    video_signal_parameters_s p = kvideoparam_parameters_for_resolution(r);

    // Apply the parameters to the current input signal.
    /// TODO. Add error-checking.
    RGBSetPhase(this->captureHandle,         p.phase);
    RGBSetBlackLevel(this->captureHandle,    p.blackLevel);
    RGBSetHorScale(this->captureHandle,      p.horizontalScale);
    RGBSetHorPosition(this->captureHandle,   p.horizontalPosition);
    RGBSetVerPosition(this->captureHandle,   p.verticalPosition);
    RGBSetBrightness(this->captureHandle,    p.overallBrightness);
    RGBSetContrast(this->captureHandle,      p.overallContrast);
    RGBSetColourBalance(this->captureHandle, p.redBrightness,
                                             p.greenBrightness,
                                             p.blueBrightness,
                                             p.redContrast,
                                             p.greenContrast,
                                             p.blueContrast);

    (void)p;

    return true;
}

void capture_api_rgbeasy_s::set_video_signal_parameters(const video_signal_parameters_s p)
{
    if (this->has_no_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));

        return;
    }

    RGBSetPhase(this->captureHandle,         p.phase);
    RGBSetBlackLevel(this->captureHandle,    p.blackLevel);
    RGBSetHorPosition(this->captureHandle,   p.horizontalPosition);
    RGBSetHorScale(this->captureHandle,      p.horizontalScale);
    RGBSetVerPosition(this->captureHandle,   p.verticalPosition);
    RGBSetBrightness(this->captureHandle,    p.overallBrightness);
    RGBSetContrast(this->captureHandle,      p.overallContrast);
    RGBSetColourBalance(this->captureHandle, p.redBrightness,
                                             p.greenBrightness,
                                             p.blueBrightness,
                                             p.redContrast,
                                             p.greenContrast,
                                             p.blueContrast);

    kvideoparam_update_parameters_for_resolution(this->get_resolution(), p);

    return;
}

bool capture_api_rgbeasy_s::adjust_horizontal_offset(const int delta)
{
    if (!delta)
    {
        return true;
    }

    if (!RECEIVING_A_SIGNAL)
    {
        return false;
    }

    const long newPos = (this->get_video_signal_parameters().horizontalPosition + delta);
    if (newPos < this->get_minimum_video_signal_parameters().horizontalPosition ||
        newPos > this->get_maximum_video_signal_parameters().horizontalPosition)
    {
        return false;
    }

    if (apicall_succeeded(RGBSetHorPosition(this->captureHandle, newPos)))
    {
        // Assume that this was a user-requested change, and as such that it
        // should affect the user's custom mode parameter settings.
        kvideoparam_update_parameters_for_resolution(this->get_resolution(), this->get_video_signal_parameters());

        kd_update_video_mode_params();
    }

    return true;
}

bool capture_api_rgbeasy_s::adjust_vertical_offset(const int delta)
{
    if (!delta) return true;
    if (!RECEIVING_A_SIGNAL) return false;

    const long newPos = (this->get_video_signal_parameters().verticalPosition + delta);
    if (newPos < std::max(2, (int)this->get_minimum_video_signal_parameters().verticalPosition) ||
        newPos > this->get_maximum_video_signal_parameters().verticalPosition)
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
        kvideoparam_update_parameters_for_resolution(this->get_resolution(), this->get_video_signal_parameters());

        kd_update_video_mode_params();
    }

    return true;
}

bool capture_api_rgbeasy_s::set_input_channel(const unsigned idx)
{
    const int numInputChannels = this->get_device_max_input_count();

    if (numInputChannels < 0)
    {
        NBENE(("Encountered an unexpected error while setting the input channel. Aborting the action."));

        goto fail;
    }

    if (idx >= (unsigned)numInputChannels)
    {
        INFO(("Was asked to set an input channel that is out of bounds. Ignoring the request."));

        goto fail;
    }

    if (apicall_succeeded(RGBSetInput(this->captureHandle, idx)))
    {
        INFO(("Setting capture input channel to %u.", (idx + 1)));

        INPUT_CHANNEL_IDX = idx;
    }
    else
    {
        NBENE(("Failed to set capture input channel to %u.", (idx + 1)));

        goto fail;
    }

    return true;

    fail:
    return false;
}

bool capture_api_rgbeasy_s::set_pixel_format(const capture_pixel_format_e pf)
{
    if (apicall_succeeded(RGBSetPixelFormat(this->captureHandle, pixel_format_to_rgbeasy_pixel_format(pf))))
    {
        CAPTURE_PIXEL_FORMAT = pf;
        
        return true;
    }

    return false;
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

    const auto currentInputRes = this->get_resolution();
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

    return true;

    fail:
    return false;
}

uint capture_api_rgbeasy_s::get_missed_frames_count(void) const
{
    return NUM_NEW_FRAME_EVENTS_SKIPPED;
}

uint capture_api_rgbeasy_s::get_input_channel_idx(void) const
{
    return INPUT_CHANNEL_IDX;
}

uint capture_api_rgbeasy_s::get_color_depth(void) const
{
    switch (CAPTURE_PIXEL_FORMAT)
    {
        case capture_pixel_format_e::rgb_888: return 32;
        case capture_pixel_format_e::rgb_565: return 16;
        case capture_pixel_format_e::rgb_555: return 16;
        default: k_assert(0, "Unknown capture pixel format."); return 0;
    }
}

bool capture_api_rgbeasy_s::is_capturing(void) const
{
    return this->captureIsActive;
}

bool capture_api_rgbeasy_s::has_invalid_signal(void) const
{
    return IS_SIGNAL_INVALID;
}

bool capture_api_rgbeasy_s::has_no_signal(void) const
{
    return !RECEIVING_A_SIGNAL;
}

capture_pixel_format_e capture_api_rgbeasy_s::get_pixel_format(void) const
{
    return CAPTURE_PIXEL_FORMAT;
}
