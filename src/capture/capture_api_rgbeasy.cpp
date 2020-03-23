#include <atomic>
#include <cmath>
#include "common/globals.h"
#include "common/propagate.h"
#include "capture/capture_api_rgbeasy.h"
#include "capture/capture.h"
#include "capture/alias.h"

// Set to true upon first receiving a signal after 'no signal'.
static bool SIGNAL_WOKE_UP = false;

// If set to true, the scaler should skip the next frame we send.
static unsigned SKIP_NEXT_NUM_FRAMES = false;

static std::vector<video_mode_params_s> KNOWN_MODES;

// The color depth/format in which the capture hardware captures the frames.
static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::RGB_888;

// The color depth in which the capture hardware is expected to be sending the
// frames. This depends on the current pixel format, such that e.g. formats
// 555 and 565 are probably sent as 16-bit, and 888 as 32-bit.
static unsigned CAPTURE_OUTPUT_COLOR_DEPTH = 32;

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

// Set to true if the capture hardware's input mode changes.
static bool RECEIVED_NEW_VIDEO_MODE = false;

// The maximum image depth that the capturer can handle.
static const unsigned MAX_BIT_DEPTH = 32;

// Set to 1 if we've acquired access to the RGBEASY API.
static bool RGBEASY_IS_LOADED = 0;

static HRGB CAPTURE_HANDLE = 0;
static HRGBDLL RGBAPI_HANDLE = 0;

// Set to 1 if we're currently capturing.
static bool CAPTURE_IS_ACTIVE = false;

bool capture_api_rgbeasy_s::initialize(void)
{
    INFO(("Initializing the capture API."));

    this->frameBuffer.pixels.alloc(MAX_FRAME_SIZE);

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

    this->frameBuffer.pixels.release_memory();

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

    RGBEASY_IS_LOADED = false;

    return true;
}

PIXELFORMAT capture_api_rgbeasy_s::pixel_format_to_rgbeasy_pixel_format(capture_pixel_format_e fmt)
{
    switch (fmt)
    {
        case capture_pixel_format_e::RGB_555: return RGB_PIXELFORMAT_555;
        case capture_pixel_format_e::RGB_565: return RGB_PIXELFORMAT_565;
        case capture_pixel_format_e::RGB_888: return RGB_PIXELFORMAT_888;
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
    else RGBEASY_IS_LOADED = true;

    if (!apicall_succeeded(RGBOpenInput(INPUT_CHANNEL_IDX, &CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetFrameDropping(CAPTURE_HANDLE, FRAME_SKIP)) ||
        !apicall_succeeded(RGBSetDMADirect(CAPTURE_HANDLE, FALSE)) ||
        !apicall_succeeded(RGBSetPixelFormat(CAPTURE_HANDLE, pixel_format_to_rgbeasy_pixel_format(CAPTURE_PIXEL_FORMAT))) ||
        !apicall_succeeded(RGBUseOutputBuffers(CAPTURE_HANDLE, FALSE)) ||
        !apicall_succeeded(RGBSetFrameCapturedFn(CAPTURE_HANDLE, rgbeasyCallbacks.frame_captured, 0)) ||
        !apicall_succeeded(RGBSetModeChangedFn(CAPTURE_HANDLE, rgbeasyCallbacks.video_mode_changed, 0)) ||
        !apicall_succeeded(RGBSetInvalidSignalFn(CAPTURE_HANDLE, rgbeasyCallbacks.invalid_signal, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetErrorFn(CAPTURE_HANDLE, rgbeasyCallbacks.error, (ULONG_PTR)&CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetNoSignalFn(CAPTURE_HANDLE, rgbeasyCallbacks.no_signal, (ULONG_PTR)&CAPTURE_HANDLE)))
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

const captured_frame_s& capture_api_rgbeasy_s::get_frame_buffer(void) const
{
    return this->frameBuffer;
}

// Examine the state of the capture system and decide which has been the most recent
// capture event.
//
/// FIXME: This is a bit of an ugly way to handle things. For instance, the function
/// is a getter, but also modifies the unit's state.
capture_event_e capture_api_rgbeasy_s::get_latest_capture_event(void) const
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

    // If there were no events that we should notify the caller about.
    return capture_event_e::none;
}

capture_signal_s capture_api_rgbeasy_s::get_signal_info(void) const
{
    if (kc_no_signal())
    {
        NBENE(("Tried to query the capture signal while no signal was being received."));
        return {0};
    }

    capture_signal_s s = {0};

    RGBMODEINFO mi = {0};
    mi.Size = sizeof(mi);

    s.wokeUp = SIGNAL_WOKE_UP;

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
    INFO(("Applying mode parameters for %u x %u.", r.w, r.h));

    video_mode_params_s p = kc_mode_params_for_resolution(r);

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

    update_known_video_mode_params(this->get_resolution(), &c, nullptr);

    return;
}

void capture_api_rgbeasy_s::set_video_settings(const capture_video_settings_s v)
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

void capture_api_rgbeasy_s::report_frame_buffer_processing_finished(void)
{
    CNT_FRAMES_PROCESSED = CNT_FRAMES_CAPTURED.load();

    SKIP_NEXT_NUM_FRAMES -= bool(SKIP_NEXT_NUM_FRAMES);

    this->frameBuffer.processed = true;

    return;
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
        case 24: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::RGB_888; CAPTURE_OUTPUT_COLOR_DEPTH = 32; break;
        case 16: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::RGB_565; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
        case 15: CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::RGB_555; CAPTURE_OUTPUT_COLOR_DEPTH = 16; break;
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
    resolution_s currentRes = this->get_resolution();
    resolution_s aliasedRes = ka_aliased(currentRes);

    // If the current resolution has an alias, switch to that.
    if ((currentRes.w != aliasedRes.w) ||
        (currentRes.h != aliasedRes.h))
    {
        if (!kc_set_resolution(aliasedRes))
        {
            NBENE(("Failed to apply an alias."));
        }
        else currentRes = aliasedRes;
    }

    kc_set_mode_parameters_for_resolution(currentRes);

    RECEIVED_NEW_VIDEO_MODE = false;

    INFO(("Capturer reports new input mode: %u x %u.", currentRes.w, currentRes.h));

    return;
}

void capture_api_rgbeasy_s::reset_missed_frames_count(void)
{
    CNT_FRAMES_SKIPPED = 0;

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

    const auto currentInputRes = kc_hardware().status.capture_resolution();
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

// Callback functions for the RGBEasy API, through which the API communicates
// with VCS.
#ifdef _WIN32
    // Called by the capture hardware when a new frame has been captured. The
    // captured RGBA data is in frameData.
    void RGBCBKAPI capture_api_rgbeasy_s::rgbeasy_callbacks_s::frame_captured(HWND,
                                                                              HRGB,
                                                                              LPBITMAPINFOHEADER frameInfo,
                                                                              void *frameData,
                                                                              ULONG_PTR)
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

        if (this->frameBuffer.pixels.is_null())
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

        this->frameBuffer.r.w = frameInfo->biWidth;
        this->frameBuffer.r.h = abs(frameInfo->biHeight);
        this->frameBuffer.r.bpp = frameInfo->biBitCount;

        // Copy the frame's data into our local buffer so we can work on it.
        memcpy(this->frameBuffer.pixels.ptr(), (u8*)frameData,
               this->frameBuffer.pixels.up_to(this->frameBuffer.r.w * this->frameBuffer.r.h * (this->frameBuffer.r.bpp / 8)));

        done:
        CNT_FRAMES_CAPTURED++;
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI capture_api_rgbeasy_s::rgbeasy_callbacks_s::video_mode_changed(HWND,
                                                                                  HRGB,
                                                                                  PRGBMODECHANGEDINFO,
                                                                                  ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        SIGNAL_WOKE_UP = !RECEIVING_A_SIGNAL;
        RECEIVED_NEW_VIDEO_MODE = true;

        done:
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI capture_api_rgbeasy_s::rgbeasy_callbacks_s::invalid_signal(HWND,
                                                                              HRGB,
                                                                              unsigned long horClock,
                                                                              unsigned long verClock,
                                                                              ULONG_PTR captureHandle)
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
    void RGBCBKAPI capture_api_rgbeasy_s::rgbeasy_callbacks_s::no_signal(HWND,
                                                                         HRGB,
                                                                         ULONG_PTR captureHandle)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        // Let the card apply its own 'no signal' handler as well, just in case.
        RGBNoSignal(captureHandle);

        SIGNAL_WAS_LOST = true;

        return;
    }

    void RGBCBKAPI capture_api_rgbeasy_s::rgbeasy_callbacks_s::error(HWND,
                                                                     HRGB,
                                                                     unsigned long,
                                                                     ULONG_PTR,
                                                                     unsigned long*)
    {
        std::lock_guard<std::mutex> lock(INPUT_OUTPUT_MUTEX);

        UNRECOVERABLE_CAPTURE_ERROR = true;

        return;
    }
#endif

uint capture_api_rgbeasy_s::get_num_missed_frames(void)
{
    return CNT_FRAMES_SKIPPED;
}

uint capture_api_rgbeasy_s::get_input_channel_idx(void)
{
    return INPUT_CHANNEL_IDX;
}

uint capture_api_rgbeasy_s::get_input_color_depth(void)
{
    switch (CAPTURE_PIXEL_FORMAT)
    {
        case capture_pixel_format_e::RGB_888: return 24;
        case capture_pixel_format_e::RGB_565: return 16;
        case capture_pixel_format_e::RGB_555: return 15;
        default: k_assert(0, "Unknown capture pixel format."); return 0;
    }
}

bool capture_api_rgbeasy_s::get_are_frames_being_dropped(void)
{
    return bool(CNT_FRAMES_SKIPPED > 0);
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
    return SIGNAL_IS_INVALID;
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

    INFO(("Unknown video mode; returning default parameters."));
    return {r,
            this->get_default_color_settings(),
            this->get_default_video_settings()};
}
