/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * Implements capturing under Linux via the Datapath's Vision driver and the
 * Vision4Linux 2 API.
 *
 * Initialization happens in ::initialize(). The capture thread runs in
 * capture_function().
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

#include <unordered_map>
#include <cmath>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <chrono>
#include <poll.h>
#include "capture/capture_api_video4linux.h"
#include "capture/input_channel_v4l.h"
#include "capture/ic_v4l_video_parameters.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133v4l2.h>

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// A back buffer area for the capture device to capture into.
static capture_back_buffer_s CAPTURE_BACK_BUFFER;

capture_event_e capture_api_video4linux_s::pop_capture_event_queue(void)
{
    if (!this->inputChannel)
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::new_video_mode))
    {
        this->set_resolution(this->get_source_resolution());

        this->inputChannel->captureStatus.videoParameters.update();

        return capture_event_e::new_video_mode;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::signal_lost))
    {
        this->inputChannel->captureStatus.videoParameters.update();

        return capture_event_e::signal_lost;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::invalid_device))
    {
        return capture_event_e::invalid_device;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }
    else if (this->inputChannel->pop_capture_event(capture_event_e::sleep))
    {
        return capture_event_e::sleep;
    }

    return capture_event_e::none;
}

resolution_s capture_api_video4linux_s::get_resolution(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.resolution;
}

resolution_s capture_api_video4linux_s::get_minimum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    return resolution_s{MIN_OUTPUT_WIDTH, MIN_OUTPUT_HEIGHT, 32};
}

resolution_s capture_api_video4linux_s::get_maximum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    return resolution_s{1920, 1080, 32};
}

refresh_rate_s capture_api_video4linux_s::get_refresh_rate(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.refreshRate;
}

uint capture_api_video4linux_s::get_missed_frames_count(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.numNewFrameEventsSkipped;
}

bool capture_api_video4linux_s::has_invalid_signal() const
{
    /// TODO.
    return false;
}

bool capture_api_video4linux_s::has_invalid_device() const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.invalidDevice;
}

bool capture_api_video4linux_s::has_no_signal(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.noSignal;
}

capture_pixel_format_e capture_api_video4linux_s::get_pixel_format() const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    return this->inputChannel->captureStatus.pixelFormat;
}

resolution_s capture_api_video4linux_s::get_source_resolution(void) const
{
    k_assert((this->inputChannel),
             "Attempting to set input channel parameters on a null channel.");

    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (!this->inputChannel->device_ioctl(RGB133_VIDIOC_G_SRC_FMT, &format))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    return {format.fmt.pix.width,
            format.fmt.pix.height,
            32}; /// TODO: Don't assume the bit depth.
}

bool capture_api_video4linux_s::set_resolution(const resolution_s &r)
{
    k_assert((this->inputChannel),
             "Attempting to set input channel parameters on a null channel.");

    v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!this->inputChannel->device_ioctl(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to query the current capture format (error %d).", errno));

        goto fail;
    }

    format.fmt.pix.width = r.w;
    format.fmt.pix.height = r.h;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;

    if (!this->inputChannel->device_ioctl(VIDIOC_S_FMT, &format) ||
        !this->inputChannel->device_ioctl(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to set the current capture format (error %d).", errno));

        goto fail;
    }

    k_assert((format.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32),
             "Invalid capture pixel format.");

    return true;

    fail:
    return false;
}

bool capture_api_video4linux_s::initialize(void)
{
    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels.alloc(MAX_FRAME_SIZE);

    CAPTURE_BACK_BUFFER.allocate();

    this->set_input_channel(INPUT_CHANNEL_IDX);

    return true;

    fail:
    PROGRAM_EXIT_REQUESTED = 1;
    return false;
}

bool capture_api_video4linux_s::release(void)
{
    delete this->inputChannel;

    FRAME_BUFFER.pixels.release_memory();
    CAPTURE_BACK_BUFFER.release();

    return true;
}

std::string capture_api_video4linux_s::get_device_name(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    v4l2_capability caps = {};

    if (!this->inputChannel->device_ioctl(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (char*)caps.card;
}

std::string capture_api_video4linux_s::get_device_driver_version(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    v4l2_capability caps = {};

    if (!this->inputChannel->device_ioctl(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (std::string((char*)caps.driver)             + " " +
            std::to_string((caps.version >> 16) & 0xff) + "." +
            std::to_string((caps.version >> 8) & 0xff)  + "." +
            std::to_string((caps.version >> 0) & 0xff));
}

std::string capture_api_video4linux_s::get_device_firmware_version(void) const
{
    return "Unknown";
}


int capture_api_video4linux_s::get_device_maximum_input_count(void) const
{
    const char baseDevicePath[] = "/dev/video";
    int numInputs = 0;

    // Returns true if the given V4L capabilities appear to indicate a Vision
    // capture device.
    const auto is_vision_capture_device = [](const v4l2_capability *const caps) 
    {
        const bool usesVisionDriver = (strcmp((const char*)caps->driver, "Vision") == 0);
        const bool isVisionControlDevice = (strcmp((const char*)caps->card, "Vision Control") == 0);

        return (usesVisionDriver && !isVisionControlDevice);
    };

    /// TODO: Make sure we only count inputs on the capture card that we're
    /// currently capturing on, rather than tallying up the number of inputs on
    /// all the Vision capture cards on the system.
    for (int i = 0; i < 64; i++) 
    {
        v4l2_capability deviceCaps; 

        const int deviceFile = open((baseDevicePath + std::to_string(i)).c_str(), O_RDONLY);
        
        if (deviceFile < 0)
        {
            continue;
        }

        if ((ioctl(deviceFile, VIDIOC_QUERYCAP, &deviceCaps) >= 0) &&
            is_vision_capture_device(&deviceCaps))
        {
            numInputs++;
        }

        close(deviceFile);
    }
    
    return numInputs;
}

video_signal_parameters_s capture_api_video4linux_s::get_video_signal_parameters(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    if (this->has_no_signal())
    {
        return this->get_default_video_signal_parameters();
    }

    const auto videoParams = this->inputChannel->captureStatus.videoParameters;

    video_signal_parameters_s p;

    p.phase              = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::phase);
    p.blackLevel         = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::black_level);
    p.horizontalPosition = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_position);
    p.verticalPosition   = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::vertical_position);
    p.horizontalScale    = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_size);
    p.overallBrightness  = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::brightness);
    p.overallContrast    = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::contrast);
    p.redBrightness      = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::red_brightness);
    p.greenBrightness    = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::green_brightness);
    p.blueBrightness     = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::blue_brightness);
    p.redContrast        = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::red_contrast);
    p.greenContrast      = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::green_contrast);
    p.blueContrast       = videoParams.value(ic_v4l_video_parameters_c::parameter_type_e::blue_contrast);

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_default_video_signal_parameters(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = this->inputChannel->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 0;
        p.blackLevel         = 8;
        p.horizontalPosition = 112;
        p.verticalPosition   = 11;
        p.horizontalScale    = 800;
        p.overallBrightness  = 32;
        p.overallContrast    = 128;
        p.redBrightness      = 128;
        p.greenBrightness    = 256;
        p.blueBrightness     = 128;
        p.redContrast        = 256;
        p.greenContrast      = 128;
        p.blueContrast       = 256;
    }
    else
    {
        p.phase              = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::phase);
        p.blackLevel         = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::black_level);
        p.horizontalPosition = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_position);
        p.verticalPosition   = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::vertical_position);
        p.horizontalScale    = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_size);
        p.overallBrightness  = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::brightness);
        p.overallContrast    = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::contrast);
        p.redBrightness      = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::red_brightness);
        p.greenBrightness    = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::green_brightness);
        p.blueBrightness     = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::blue_brightness);
        p.redContrast        = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::red_contrast);
        p.greenContrast      = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::green_contrast);
        p.blueContrast       = videoParams.default_value(ic_v4l_video_parameters_c::parameter_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_minimum_video_signal_parameters(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = this->inputChannel->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 0;
        p.blackLevel         = 1;
        p.horizontalPosition = 32;
        p.verticalPosition   = 4;
        p.horizontalScale    = 100;
        p.overallBrightness  = 0;
        p.overallContrast    = 0;
        p.redBrightness      = 0;
        p.greenBrightness    = 0;
        p.blueBrightness     = 0;
        p.redContrast        = 0;
        p.greenContrast      = 0;
        p.blueContrast       = 0;
    }
    else
    {
        p.phase              = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::phase);
        p.blackLevel         = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::black_level);
        p.horizontalPosition = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_position);
        p.verticalPosition   = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::vertical_position);
        p.horizontalScale    = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_size);
        p.overallBrightness  = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::brightness);
        p.overallContrast    = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::contrast);
        p.redBrightness      = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::red_brightness);
        p.greenBrightness    = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::green_brightness);
        p.blueBrightness     = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::blue_brightness);
        p.redContrast        = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::red_contrast);
        p.greenContrast      = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::green_contrast);
        p.blueContrast       = videoParams.minimum_value(ic_v4l_video_parameters_c::parameter_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_maximum_video_signal_parameters(void) const
{
    k_assert((this->inputChannel),
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = this->inputChannel->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (this->has_no_signal())
    {
        p.phase              = 31;
        p.blackLevel         = 255;
        p.horizontalPosition = 800;
        p.verticalPosition   = 15;
        p.horizontalScale    = 4095;
        p.overallBrightness  = 255;
        p.overallContrast    = 255;
        p.redBrightness      = 255;
        p.greenBrightness    = 511;
        p.blueBrightness     = 255;
        p.redContrast        = 511;
        p.greenContrast      = 255;
        p.blueContrast       = 511;
    }
    else
    {
        p.phase              = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::phase);
        p.blackLevel         = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::black_level);
        p.horizontalPosition = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_position);
        p.verticalPosition   = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::vertical_position);
        p.horizontalScale    = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::horizontal_size);
        p.overallBrightness  = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::brightness);
        p.overallContrast    = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::contrast);
        p.redBrightness      = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::red_brightness);
        p.greenBrightness    = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::green_brightness);
        p.blueBrightness     = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::blue_brightness);
        p.redContrast        = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::red_contrast);
        p.greenContrast      = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::green_contrast);
        p.blueContrast       = videoParams.maximum_value(ic_v4l_video_parameters_c::parameter_type_e::blue_contrast);
    }

    return p;
}

const captured_frame_s& capture_api_video4linux_s::get_frame_buffer(void) const
{
    return FRAME_BUFFER;
}

bool capture_api_video4linux_s::mark_frame_buffer_as_processed(void)
{
    k_assert((this->inputChannel),
             "Attempting to set input channel parameters on a null channel.");

    this->inputChannel->captureStatus.numFramesProcessed++;

    FRAME_BUFFER.processed = true;

    return true;
}

bool capture_api_video4linux_s::reset_missed_frames_count()
{
    k_assert((this->inputChannel),
             "Attempting to set input channel parameters on a null channel.");

    this->inputChannel->captureStatus.numNewFrameEventsSkipped = 0;

    return true;
}

bool capture_api_video4linux_s::set_input_channel(const unsigned idx)
{
    if (this->inputChannel)
    {
        delete this->inputChannel;
    }

    const std::string captureDeviceFileName = (std::string("/dev/video") + std::to_string(idx));
    this->inputChannel = new input_channel_v4l_c(this,
                                                 captureDeviceFileName,
                                                 &FRAME_BUFFER,
                                                 &CAPTURE_BACK_BUFFER);
    return true;
}

bool capture_api_video4linux_s::set_video_signal_parameters(const video_signal_parameters_s &p)
{
    k_assert((this->inputChannel),
             "Attempting to set input channel parameters on a null channel.");

    if (this->has_no_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));

        return true;
    }

    auto videoParams = this->inputChannel->captureStatus.videoParameters;

    const auto set_parameter = [&videoParams](const int value, const ic_v4l_video_parameters_c::parameter_type_e parameterType)
    {
        if (videoParams.value(parameterType) != value)
        {
            return videoParams.set_value(value, parameterType);
        }

        return true;
    };

    set_parameter(p.phase,              ic_v4l_video_parameters_c::parameter_type_e::phase);
    set_parameter(p.blackLevel,         ic_v4l_video_parameters_c::parameter_type_e::black_level);
    set_parameter(p.horizontalPosition, ic_v4l_video_parameters_c::parameter_type_e::horizontal_position);
    set_parameter(p.verticalPosition,   ic_v4l_video_parameters_c::parameter_type_e::vertical_position);
    set_parameter(p.horizontalScale,    ic_v4l_video_parameters_c::parameter_type_e::horizontal_size);
    set_parameter(p.overallBrightness,  ic_v4l_video_parameters_c::parameter_type_e::brightness);
    set_parameter(p.overallContrast,    ic_v4l_video_parameters_c::parameter_type_e::contrast);
    set_parameter(p.redBrightness,      ic_v4l_video_parameters_c::parameter_type_e::red_brightness);
    set_parameter(p.greenBrightness,    ic_v4l_video_parameters_c::parameter_type_e::green_brightness);
    set_parameter(p.blueBrightness,     ic_v4l_video_parameters_c::parameter_type_e::blue_brightness);
    set_parameter(p.redContrast,        ic_v4l_video_parameters_c::parameter_type_e::red_contrast);
    set_parameter(p.greenContrast,      ic_v4l_video_parameters_c::parameter_type_e::green_contrast);
    set_parameter(p.blueContrast,       ic_v4l_video_parameters_c::parameter_type_e::blue_contrast);

    videoParams.update();

    return true;
}

#endif
