/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

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

#define INCLUDE_VISION
#include <visionrgb/include/rgb133control.h>
#include <visionrgb/include/rgb133v4l2.h>

static std::atomic<unsigned int> NUM_FRAMES_PROCESSED(0);
static std::atomic<unsigned int> NUM_FRAMES_CAPTURED(0);

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and so which we had to skip.
static std::atomic<unsigned int> NUM_NEW_FRAME_EVENTS_SKIPPED(0);

// A value returned from open("/dev/videoX") - capture channel for the
// selected input.
static int CAPTURE_HANDLE = 0;

// A value returned from open("/dev/video63") - control channel for the
// Vision API.
static int VISION_HANDLE = 0;

static bool NO_SIGNAL = false;
static bool INVALID_SIGNAL = false;

// Information about the capture device, in the Vision API's format.
static _sVWDeviceInfo DEVICE_INFO;

// The default/min/max values for the device's parameters.
static _sVWDeviceParms DEVICE_MINIMUM_PARAMS;
static _sVWDeviceParms DEVICE_MAXIMUM_PARAMS;
static _sVWDeviceParms DEVICE_DEFAULT_PARAMS;

// The current (or most recent) capture resolution. This is a cached variable
// intended to reduce calls to the Vision API - it gets updated whenever the
// API tells us of a new capture resolution without us specifically polling
// the API for it.
static resolution_s CAPTURE_RESOLUTION = {640, 480, 32};

static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// A back buffer area for the capture device to capture into.
struct capture_back_buffer_s
{
    const unsigned numPages = 2;

    heap_bytes_s<u8>& page(const unsigned idx)
    {
        k_assert((idx < this->numPages), "Accessing back buffer pages out of bounds.");
        return this->pages[idx];
    }

    void allocate(void)
    {
        for (auto &page: this->pages)
        {
            page.alloc(MAX_FRAME_SIZE);
        }
    }

    void release(void)
    {
        for (auto &page: this->pages)
        {
            if (!page.is_null())
            {
                page.release_memory();
            }
        }
    }

    capture_back_buffer_s(void)
    {
        this->pages.resize(this->numPages);
    }

    ~capture_back_buffer_s(void)
    {
        this->release();
    }

private:
    std::vector<heap_bytes_s<u8>> pages;
} CAPTURE_BACK_BUFFER;

// Flags that the capture thread will set to convey to the VCS thread the
// various capture events it detects during capture.
static bool CAPTURE_EVENT_FLAG[static_cast<int>(capture_event_e::num_enumerators)] = {false};

static bool vision_read_device_info(_sVWDeviceInfo *const deviceInfo)
{
    k_assert(VISION_HANDLE, "Attempting to query the Vision API before initializing it.");

    deviceInfo->magic = VW_MAGIC_DEVICE_INFO;

    if (read(VISION_HANDLE, deviceInfo, sizeof(_sVWDeviceInfo)) <= 0)
    {
       return false;
    }

    return true;
}

static bool vision_read_device_input(const unsigned inputIdx, _sVWInput *const input)
{
    k_assert(VISION_HANDLE, "Attempting to query the Vision API before initializing it.");

    _sVWDevice device;
    memset(&device, 0, sizeof(device));
    device.magic = VW_MAGIC_DEVICE;
    device.input = inputIdx;
    device.device = 0;

    if (read(VISION_HANDLE, &device, sizeof(_sVWDevice)) <= 0)
    {
        return false;
    }

    *input = device.inputs[inputIdx];

    return true;
}

static bool vision_read_device_params(_sVWDeviceParms *const deviceParams)
{
    _sVWInput input;

    if (!vision_read_device_input(0, &input))
    {
        return false;
    }

    *deviceParams = input.curDeviceParms;

    return true;
}

static bool vision_read_device_default_params(_sVWDeviceParms *const deviceParams)
{
    _sVWInput input;
    vision_read_device_input(0, &input);

    *deviceParams = input.defDeviceParms;

    return true;
}

static bool vision_read_device_minimum_params(_sVWDeviceParms *const deviceParams)
{
    _sVWInput input;
    vision_read_device_input(0, &input);

    *deviceParams = input.minDeviceParms;

    return true;
}

static bool vision_read_device_maximum_params(_sVWDeviceParms *const deviceParams)
{
    _sVWInput input;
    vision_read_device_input(0, &input);

    *deviceParams = input.maxDeviceParms;

    return true;
}

// Marks the given capture event as having occurred.
static void push_capture_event(capture_event_e event)
{
    CAPTURE_EVENT_FLAG[static_cast<int>(event)] = true;

    return;
}

// Removes the given capture event from the event 'queue', and returns its value.
static bool pop_capture_event(capture_event_e event)
{
    const bool eventOccurred = CAPTURE_EVENT_FLAG[static_cast<int>(event)];

    CAPTURE_EVENT_FLAG[static_cast<int>(event)] = false;

    return eventOccurred;
}

static bool capture_apicall(const unsigned long request, void *data)
{
    const int retVal = ioctl(CAPTURE_HANDLE, request, data);

    if (retVal < 0)
    {
         NBENE(("An apicall (request %d) failed (error %d).", request, retVal));

        return false;
    }

    return true;
}

// Stops the capture stream.
static bool stream_off(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_STREAMOFF, &bufType))
    {
        NBENE(("Couldn't stop the capture stream."));

        return false;
    }

    return true;
}

// Starts the capture stream.
static bool stream_on(void)
{
    v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_STREAMON, &bufType))
    {
        NBENE(("Couldn't start the capture stream."));

        return false;
    }

    return true;
}

bool capture_api_video4linux_s::unqueue_capture_buffers(void)
{
    // Tell the capture device we want it to use capture buffers we've allocated.
    {
        v4l2_requestbuffers buf;

        memset(&buf, 0, sizeof(buf));
        buf.count = 0;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (!capture_apicall(VIDIOC_REQBUFS, &buf))
        {
            NBENE(("Failed to unqueue capture buffers (error %d).", errno));

            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

bool capture_api_video4linux_s::enqueue_capture_buffers(void)
{
    // Set the capture resolution to the highest possible - it seems this is
    // needed to get the capture device to assign the maximum possible frame
    // size for the buffers.
    {
        const resolution_s maxCaptureResolution = this->get_maximum_resolution();

        v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }

        format.fmt.pix.width = maxCaptureResolution.w;
        format.fmt.pix.height = maxCaptureResolution.h;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!capture_apicall(VIDIOC_S_FMT, &format) ||
            !capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to set the current capture format for enqueuing capture buffers(error %d).", errno));
            goto fail;
        }

        if ((format.fmt.pix.width != maxCaptureResolution.w) ||
            (format.fmt.pix.height != maxCaptureResolution.h) ||
            (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB32))
        {
            NBENE(("Failed to initialize the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }
    }

    // Tell the capture device that we've allocated frame buffers for it to use.
    {
        v4l2_requestbuffers buf;

        memset(&buf, 0, sizeof(buf));
        buf.count = CAPTURE_BACK_BUFFER.numPages;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (!capture_apicall(VIDIOC_REQBUFS, &buf))
        {
            NBENE(("User pointer streaming couldn't be initialized (error %d).", errno));

            goto fail;
        }
    }

    // Enqueue the frame buffers we've allocated.
    for (unsigned i = 0; i < CAPTURE_BACK_BUFFER.numPages; i++)
    {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)CAPTURE_BACK_BUFFER.page(i).ptr();
        buf.length = CAPTURE_BACK_BUFFER.page(i).size();

        if (!capture_apicall(VIDIOC_QBUF, &buf))
        {
            NBENE(("Failed to enqueue capture buffers (failed on buffer #%d).", (i + 1)));

            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

// Runs in a separate thread to poll for capture events from the capture device.
// The thread will be terminated externally when the program exits.
static void capture_function(capture_api_video4linux_s *const thisPtr)
{
    // We'll keep track of the current device parameters to detect changes in
    // video mode.
    _sVWDeviceParms knownDeviceParams = {};
    knownDeviceParams.type = _eVWSignalType::VW_TYPE_UNKNOWN; // Make sure we correctly trigger the current signal type on the first loop.

    while (!PROGRAM_EXIT_REQUESTED)
    {
        // See if aspects of the signal has changed.
        /// TODO: Are there signal events we could hook onto, rather than
        ///       constantly polling for these parameters?
        {
            _sVWDeviceParms updatedDeviceParams;

            if (vision_read_device_params(&updatedDeviceParams))
            {
                if ((updatedDeviceParams.type != knownDeviceParams.type) ||                                           // Lost or gained a signal.
                    (updatedDeviceParams.VideoTimings.VerFrequency != knownDeviceParams.VideoTimings.VerFrequency) || // Refresh rate changed.
                    (updatedDeviceParams.VideoTimings.HorAddrTime != knownDeviceParams.VideoTimings.HorAddrTime) ||   // Horizontal resolution changed.
                    (updatedDeviceParams.VideoTimings.VerAddrTime != knownDeviceParams.VideoTimings.VerAddrTime))     // Vertical resolution changed.
                {
                    std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

                    knownDeviceParams.type = updatedDeviceParams.type;
                    knownDeviceParams.VideoTimings.VerFrequency = updatedDeviceParams.VideoTimings.VerFrequency;
                    knownDeviceParams.VideoTimings.HorAddrTime = updatedDeviceParams.VideoTimings.HorAddrTime;
                    knownDeviceParams.VideoTimings.VerAddrTime = updatedDeviceParams.VideoTimings.VerAddrTime;

                    if (knownDeviceParams.type == VW_TYPE_NOSIGNAL)
                    {
                        NO_SIGNAL = true;

                        push_capture_event(capture_event_e::signal_lost);
                    }
                    else
                    {
                        NO_SIGNAL = false;

                        push_capture_event(capture_event_e::new_video_mode);
                    }
                }
            }
        }

        if (thisPtr->has_no_signal())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            continue;
        }

        // Poll the capture device for a new frame.
        {
            pollfd fd;
            memset(&fd, 0, sizeof(fd));
            fd.fd = CAPTURE_HANDLE;
            fd.events = (POLLIN | POLLPRI);

            const int pollResult = poll(&fd, 1, 1000);

            // Received a new frame.
            if (pollResult > 0)
            {
                if (!(fd.revents & (POLLIN | POLLPRI)))
                {
                    continue;
                }

                v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                // Tell the capture device we want to access its capture buffer.
                if (!capture_apicall(VIDIOC_DQBUF, &buf))
                {
                    switch (errno)
                    {
                        case EAGAIN: continue;
                        default: push_capture_event(capture_event_e::unrecoverable_error); break;
                    }
                }

                // If the hardware is sending us a new frame while we're still unfinished
                // processing the previous frame, we'll skip this new frame.
                if (NUM_FRAMES_CAPTURED != NUM_FRAMES_PROCESSED)
                {
                    NUM_NEW_FRAME_EVENTS_SKIPPED++;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

                    FRAME_BUFFER.r = CAPTURE_RESOLUTION;
                    FRAME_BUFFER.pixelFormat = CAPTURE_PIXEL_FORMAT; /// TODO: Set correct pixel format.

                    // Copy the frame's data into our local buffer so we can work on it.
                    memcpy(FRAME_BUFFER.pixels.ptr(), (char*)buf.m.userptr,
                           FRAME_BUFFER.pixels.up_to(FRAME_BUFFER.r.w * FRAME_BUFFER.r.h * (FRAME_BUFFER.r.bpp / 8)));

                    NUM_FRAMES_CAPTURED++;
                    push_capture_event(capture_event_e::new_frame);
                }

                // Tell the capture device it can use its capture buffer again.
                if (!capture_apicall(VIDIOC_QBUF, &buf))
                {
                    push_capture_event(capture_event_e::unrecoverable_error);
                    break;
                }
            }
            // A capture error.
            else
            {
                std::lock_guard<std::mutex> lock(thisPtr->captureMutex);

                push_capture_event(capture_event_e::unrecoverable_error);
            }
        }
    }

    return;
}

capture_event_e capture_api_video4linux_s::pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        this->set_resolution(this->get_source_resolution());

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
    return (this->has_no_signal()? capture_event_e::sleep : capture_event_e::none);
}

resolution_s capture_api_video4linux_s::get_resolution(void) const
{
    return CAPTURE_RESOLUTION;
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

unsigned capture_api_video4linux_s::get_refresh_rate(void) const
{
    return round(get_refresh_rate_exact());
}

double capture_api_video4linux_s::get_refresh_rate_exact(void) const
{
    _sVWDeviceParms params;

    vision_read_device_params(&params);

    return (params.VideoTimings.VerFrequency / 1000.0);
}

uint capture_api_video4linux_s::get_missed_frames_count(void) const
{
    return NUM_NEW_FRAME_EVENTS_SKIPPED;
}

bool capture_api_video4linux_s::has_invalid_signal() const
{
    return INVALID_SIGNAL;
}

bool capture_api_video4linux_s::has_no_signal() const
{
    return NO_SIGNAL;
}

resolution_s capture_api_video4linux_s::get_source_resolution(void) const
{
    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (!capture_apicall(RGB133_VIDIOC_G_SRC_FMT, &format))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    return {format.fmt.pix.width,
            format.fmt.pix.height,
            32}; /// TODO: Don't assume the bit depth.
}

bool capture_api_video4linux_s::initialize_hardware(void)
{
    // Open the capture device's Vision control channel.
    if ((VISION_HANDLE = open("/dev/video63", O_RDWR)) == -1)
    {
        NBENE(("Failed to open the Vision control channel"));

        goto fail;
    }

    // Get basic device info via the Vision API.
    {
        if (!vision_read_device_info(&DEVICE_INFO) ||
            !vision_read_device_minimum_params(&DEVICE_MINIMUM_PARAMS) ||
            !vision_read_device_maximum_params(&DEVICE_MAXIMUM_PARAMS) ||
            !vision_read_device_default_params(&DEVICE_DEFAULT_PARAMS))
        {
            NBENE(("Failed to read capture device info."));

            goto fail;
        }
    }

    // Open the capture device.
    if ((CAPTURE_HANDLE = open(this->get_device_node().c_str(), O_RDWR)) < 0)
    {
        NBENE(("Failed to open the capture device."));

        goto fail;
    }

    // Verify device capabilities.
    {
        v4l2_capability caps;
        memset(&caps, 0, sizeof(caps));

        if (!capture_apicall(VIDIOC_QUERYCAP, &caps))
        {
            NBENE(("Failed to query capture device capabilities."));

            goto fail;
        }

        if (strcmp((char*)caps.card, this->get_device_name().c_str()) != 0)
        {
            NBENE(("Ambiguous capture device names: '%s' vs '%s'.", (char*)caps.card, this->get_device_name().c_str()));

            goto fail;
        }

        if (!(caps.capabilities & V4L2_CAP_READWRITE))
        {
            NBENE(("The capture device doesn't support read/write - can't do capture."));

            goto fail;
        }
    }

    if (!this->enqueue_capture_buffers())
    {
        goto fail;
    }

    // Set the starting capture resolution.
    {
        const resolution_s sourceResolution = this->get_source_resolution();

        v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));

            goto fail;
        }

        format.fmt.pix.width = sourceResolution.w;
        format.fmt.pix.height = sourceResolution.h;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!capture_apicall(VIDIOC_S_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));

            goto fail;
        }

        if (!capture_apicall(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));

            goto fail;
        }

        if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB32)
        {
            NBENE(("Failed to initialize the correct capture pixel format.", errno));

            goto fail;
        }

        CAPTURE_RESOLUTION.w = format.fmt.pix.width;
        CAPTURE_RESOLUTION.h = format.fmt.pix.height;
        CAPTURE_RESOLUTION.bpp = 32;
    }

    // Start capture.
    if (!stream_on())
    {
        goto fail;
    }

    return true;

    fail:
    NBENE(("Failed to initialize the capture device."));
    return false;
}

bool capture_api_video4linux_s::set_resolution(const resolution_s &r)
{
    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!capture_apicall(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to query the current capture format (error %d).", errno));

        goto fail;
    }

    format.fmt.pix.width = r.w;
    format.fmt.pix.height = r.h;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;

    if (!capture_apicall(VIDIOC_S_FMT, &format) ||
        !capture_apicall(VIDIOC_G_FMT, &format))
    {
        NBENE(("Failed to set the current capture format (error %d).", errno));

        goto fail;
    }

    k_assert((format.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB32),
             "Invalid capture pixel format.");

    CAPTURE_RESOLUTION.w = format.fmt.pix.width;
    CAPTURE_RESOLUTION.h = format.fmt.pix.height;
    CAPTURE_RESOLUTION.bpp = 32;

    return true;

    fail:
    return false;
}

bool capture_api_video4linux_s::release_hardware(void)
{
    bool successFlag = true;

    if (!stream_off())
    {
        successFlag = false;
    }

    return successFlag;
}

bool capture_api_video4linux_s::initialize(void)
{
    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels.alloc(MAX_FRAME_SIZE);

    CAPTURE_BACK_BUFFER.allocate();

    if (!this->initialize_hardware())
    {
        goto fail;
    }

    // Start the capture thread.
    std::thread(capture_function, this).detach();

    return true;

    fail:
    PROGRAM_EXIT_REQUESTED = 1;
    return false;
}

bool capture_api_video4linux_s::release(void)
{
    this->release_hardware();
    close(CAPTURE_HANDLE);
    close(VISION_HANDLE);

    FRAME_BUFFER.pixels.release_memory();
    CAPTURE_BACK_BUFFER.release();

    return true;
}

std::string capture_api_video4linux_s::get_device_name(void) const
{
    return DEVICE_INFO.name;
}

std::string capture_api_video4linux_s::get_device_firmware_version(void) const
{
    return (std::to_string(DEVICE_INFO.FW.Version) + "/" +
            std::to_string(DEVICE_INFO.FW.day)     + "." +
            std::to_string(DEVICE_INFO.FW.month)   + "." +
            std::to_string(DEVICE_INFO.FW.year));
}

int capture_api_video4linux_s::get_device_maximum_input_count(void) const
{
    // For now, we only support one input channel.
    return 1;
}

// Converts the Vision API's device parameter format into VCS's signal parameters
// struct.
static video_signal_parameters_s video_signal_params_from_device_params(const _sVWDeviceParms &deviceParams)
{
    video_signal_parameters_s p;

    p.r = CAPTURE_RESOLUTION;
    p.overallBrightness  = deviceParams.Brightness;
    p.overallContrast    = deviceParams.Contrast;
    p.redBrightness      = deviceParams.Colour.RedGain;
    p.redContrast        = deviceParams.Colour.RedOffset;
    p.greenBrightness    = deviceParams.Colour.GreenGain;
    p.greenContrast      = deviceParams.Colour.GreenOffset;
    p.blueBrightness     = deviceParams.Colour.BlueGain;
    p.blueContrast       = deviceParams.Colour.BlueOffset;
    p.horizontalScale    = 0; /// FIXME: Which timing parameter corresponds to this?
    p.horizontalPosition = deviceParams.VideoTimings.HorAddrTime;
    p.verticalPosition   = deviceParams.VideoTimings.VerAddrTime;
    p.phase              = deviceParams.Phase;
    p.blackLevel         = deviceParams.Blacklevel;

    return p;
}

video_signal_parameters_s capture_api_video4linux_s::get_video_signal_parameters(void) const
{
    _sVWDeviceParms params;

    vision_read_device_params(&params);

    return video_signal_params_from_device_params(params);
}

video_signal_parameters_s capture_api_video4linux_s::get_default_video_signal_parameters(void) const
{
    return video_signal_params_from_device_params(DEVICE_DEFAULT_PARAMS);
}

video_signal_parameters_s capture_api_video4linux_s::get_minimum_video_signal_parameters(void) const
{
    return video_signal_params_from_device_params(DEVICE_MINIMUM_PARAMS);
}

video_signal_parameters_s capture_api_video4linux_s::get_maximum_video_signal_parameters(void) const
{
    return video_signal_params_from_device_params(DEVICE_MAXIMUM_PARAMS);
}

std::string capture_api_video4linux_s::get_device_node(void) const
{
    return DEVICE_INFO.node;
}

const captured_frame_s& capture_api_video4linux_s::get_frame_buffer(void) const
{
    return FRAME_BUFFER;
}

bool capture_api_video4linux_s::mark_frame_buffer_as_processed(void)
{
    NUM_FRAMES_PROCESSED = NUM_FRAMES_CAPTURED.load();

    FRAME_BUFFER.processed = true;

    return true;
}

bool capture_api_video4linux_s::reset_missed_frames_count()
{
    NUM_NEW_FRAME_EVENTS_SKIPPED = 0;

    return true;
}

bool capture_api_video4linux_s::set_video_signal_parameters(const video_signal_parameters_s &p)
{
    _sVWDevice device;
    memset(&device, 0, sizeof(device));
    device.magic = VW_MAGIC_SET_DEVICE_PARMS;
    device.input = 0;
    device.device = 0;

    vision_read_device_params(&device.inputs[0].curDeviceParms);

    device.inputs[0].curDeviceParms.Brightness               = p.overallBrightness;
    device.inputs[0].curDeviceParms.Contrast                 = p.overallContrast;
    device.inputs[0].curDeviceParms.Colour.RedGain           = p.redBrightness;
    device.inputs[0].curDeviceParms.Colour.RedOffset         = p.redContrast;
    device.inputs[0].curDeviceParms.Colour.GreenGain         = p.greenBrightness;
    device.inputs[0].curDeviceParms.Colour.GreenOffset       = p.greenContrast;
    device.inputs[0].curDeviceParms.Colour.BlueGain          = p.blueBrightness;
    device.inputs[0].curDeviceParms.Colour.BlueOffset        = p.blueContrast;
    device.inputs[0].curDeviceParms.Phase                    = p.phase;
    device.inputs[0].curDeviceParms.Blacklevel               = p.blackLevel;
    device.inputs[0].curDeviceParms.VideoTimings.HorAddrTime = p.horizontalPosition;
    device.inputs[0].curDeviceParms.VideoTimings.VerAddrTime = p.verticalPosition;
    /// TODO: p.horizontalScale;

    if (write(VISION_HANDLE, &device, sizeof(device)) <= 0)
    {
        return false;
    }

    return true;
}

#endif
