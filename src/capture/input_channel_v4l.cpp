/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <linux/videodev2.h>
#include "capture/input_channel_v4l.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133v4l2.h>

input_channel_v4l_c::input_channel_v4l_c(capture_api_s *const parentCaptureAPI,
                                         const std::string v4lDeviceFileName,
                                         captured_frame_s *const dstFrameBuffer,
                                         capture_back_buffer_s *const backBuffer) :
    captureAPI(parentCaptureAPI),
    v4lDeviceFileName(v4lDeviceFileName),
    dstFrameBuffer(dstFrameBuffer),
    backBuffer(backBuffer)
{
    INFO(("Establishing a connection to %s.", this->v4lDeviceFileName.c_str()));

    this->start_capturing();

    return;
}

input_channel_v4l_c::~input_channel_v4l_c()
{
    INFO(("Closing the connection to %s.", this->v4lDeviceFileName.c_str()));

    const int retVal = this->stop_capturing();

    /// TODO: Do something with retVal if need be, e.g. report errors.
    (void)retVal;

    return;
}

void input_channel_v4l_c::push_capture_event(capture_event_e flag)
{
    const unsigned flagIdx = static_cast<unsigned>(flag);

    k_assert((flagIdx < this->captureEventFlags.size()), "Overflowing the capture event flag buffer.");

    this->captureEventFlags[flagIdx] = 1;

    return;
}

bool input_channel_v4l_c::pop_capture_event(const capture_event_e flag)
{
    const unsigned flagIdx = static_cast<unsigned>(flag);

    k_assert((flagIdx < this->captureEventFlags.size()), "Overflowing the capture event flag buffer.");

    const bool value = this->captureEventFlags[flagIdx];
    this->captureEventFlags[flagIdx] = 0;

    if ((flag == capture_event_e::sleep) &&
        !this->run)
    {
        return true;
    }
    else
    {
        return value;
    }
}

resolution_s input_channel_v4l_c::maximum_resolution(void) const
{
    /// TODO: Query actual hardware parameters for this.
    
    return resolution_s{std::min(2048u, MAX_CAPTURE_WIDTH),
                        std::min(1536u, MAX_CAPTURE_HEIGHT),
                        std::min(32u, MAX_CAPTURE_BPP)};
}

resolution_s input_channel_v4l_c::source_resolution(void)
{
    v4l2_format format;
    memset(&format, 0, sizeof(format));

    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    if (!this->device_ioctl(RGB133_VIDIOC_G_SRC_FMT, &format))
    {
        k_assert(0, "The capture hardware failed to report its input resolution.");
    }

    return {format.fmt.pix.width,
            format.fmt.pix.height,
            32}; /// TODO: Don't assume the bit depth.
}

bool input_channel_v4l_c::capture_thread__update_video_mode(void)
{
    v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_CAPTURE_SOURCE;

    // We'll set this to true, below, if need be.
    this->captureStatus.invalidSignal = false;

    /// TODO: Check for 'no signal' status.

    if (ioctl(this->v4lDeviceFileHandle, RGB133_VIDIOC_G_SRC_FMT, &format) >= 0)
    {
        if ((format.fmt.pix.width < MIN_CAPTURE_WIDTH) ||
            (format.fmt.pix.width > MAX_CAPTURE_WIDTH) ||
            (format.fmt.pix.height < MIN_CAPTURE_HEIGHT) ||
            (format.fmt.pix.height > MAX_CAPTURE_HEIGHT))
        {
            this->captureStatus.invalidSignal = true;

            return true;
        }

        // Attempt to detect if aspects of the signal have changed.
        {
            const refresh_rate_s currentRefreshRate = refresh_rate_s(format.fmt.pix.priv / 1000.0);

            if ((currentRefreshRate != this->captureStatus.refreshRate) ||
                (format.fmt.pix.width != this->captureStatus.resolution.w) ||
                (format.fmt.pix.height != this->captureStatus.resolution.h))
            {
                this->captureStatus.noSignal = false;
                this->captureStatus.refreshRate = currentRefreshRate;
                this->captureStatus.resolution.w = format.fmt.pix.width;
                this->captureStatus.resolution.h = format.fmt.pix.height;

                this->push_capture_event(capture_event_e::new_video_mode);
            }
        }
    }
    else
    {
        this->captureStatus.invalidSignal = true;

        this->push_capture_event(capture_event_e::unrecoverable_error);

        return false;
    }

    return true;
}

// Poll the capture devicve for a new frame. On success (or if no new frames
// were available), returns true. On error, returns false.
bool input_channel_v4l_c::capture_thread__get_next_frame(void)
{
    pollfd fd;
    memset(&fd, 0, sizeof(fd));
    fd.fd = this->v4lDeviceFileHandle;
    fd.events = (POLLIN | POLLPRI);

    const int pollResult = poll(&fd, 1, 1000);

    // Received a new frame.
    if (pollResult > 0)
    {
        if (!(fd.revents & (POLLIN | POLLPRI)))
        {
            return true;
        }

        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        // Tell the capture device we want to access its capture buffer.
        if (!this->device_ioctl(VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
                case EAGAIN:
                {
                    return true;
                }
                default:
                {
                    this->push_capture_event(capture_event_e::unrecoverable_error);
                    return false;
                }
            }
        }

        // If the hardware is sending us a new frame while we're still unfinished
        // processing the previous frame, we'll skip this new frame.
        if (this->captureStatus.numFramesCaptured != this->captureStatus.numFramesProcessed)
        {
            this->captureStatus.numNewFrameEventsSkipped++;
        }
        else
        {
            std::lock_guard<std::mutex> lock(captureAPI->captureMutex);

            this->dstFrameBuffer->r = this->captureStatus.resolution;
            this->dstFrameBuffer->r.bpp = ((this->captureStatus.pixelFormat == capture_pixel_format_e::rgb_888)? 32 : 16);
            this->dstFrameBuffer->pixelFormat = this->captureStatus.pixelFormat;

            // Copy the frame's data into our local buffer so we can work on it.
            memcpy(this->dstFrameBuffer->pixels.ptr(), (char*)buf.m.userptr,
                   this->dstFrameBuffer->pixels.up_to(this->dstFrameBuffer->r.w * this->dstFrameBuffer->r.h * (this->dstFrameBuffer->r.bpp / 8)));

            this->captureStatus.numFramesCaptured++;
            this->push_capture_event(capture_event_e::new_frame);
        }

        // Tell the capture device it can use its capture buffer again.
        if (!this->device_ioctl(VIDIOC_QBUF, &buf))
        {
            this->push_capture_event(capture_event_e::unrecoverable_error);
            return false;
        }
    }
    // A capture error.
    else
    {
        this->push_capture_event(capture_event_e::unrecoverable_error);
        return false;
    }

    return true;
}

bool input_channel_v4l_c::device_ioctl(const unsigned long request, void *data)
{
    if (this->v4lDeviceFileHandle < 0)
    {
        DEBUG(("Attempted to call device_ioctl() while the device file was closed. Ignoring this."));
        return false;
    }

    const int retVal = ioctl(this->v4lDeviceFileHandle, request, data);

    if (retVal < 0)
    {
        NBENE(("A call to ioctl() failed. Request: %d, error: %d.", request, retVal));
        return false;
    }

    return true;
}

bool input_channel_v4l_c::open_device(const std::string &deviceFileName)
{
    k_assert((this->v4lDeviceFileHandle < 0),
             "Attempting to re-open a capture device before having closed it.");

    this->v4lDeviceFileHandle = open(deviceFileName.c_str(), O_RDWR);

    if (this->v4lDeviceFileHandle < 0)
    {
        return false;
    }

    this->captureStatus.videoParameters = ic_v4l_video_parameters_c(this->v4lDeviceFileHandle);

    return true;
}

bool input_channel_v4l_c::close_device(void)
{
    // Already closed.
    if (this->v4lDeviceFileHandle < 0)
    {
        return true;
    }

    if (close(this->v4lDeviceFileHandle) < 0)
    {
        return false;
    }

    this->v4lDeviceFileHandle = -1;

    return true;
}

int input_channel_v4l_c::capture_thread(void)
{
    k_assert((this->v4lDeviceFileHandle >= 0),
             "Attempting to start the capture thread before the device has been opened."); 

    // Keep polling for capture events.
    while (this->run)
    {
        if (!capture_thread__update_video_mode())
        {
            return 0;
        }

        if (this->captureStatus.noSignal)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (!capture_thread__get_next_frame())
        {
            return 0;
        }
    }

    return 1;
}

u32 input_channel_v4l_c::vcs_pixel_format_to_v4l_pixel_format(capture_pixel_format_e fmt) const
{
    switch (fmt)
    {
        case capture_pixel_format_e::rgb_555: return V4L2_PIX_FMT_RGB555;
        case capture_pixel_format_e::rgb_565: return V4L2_PIX_FMT_RGB565;
        case capture_pixel_format_e::rgb_888: return V4L2_PIX_FMT_RGB32;
        default: k_assert(0, "Unknown pixel format."); return V4L2_PIX_FMT_RGB32;
    }
}

bool input_channel_v4l_c::enqueue_back_buffers(void)
{
    // Set the capture resolution to the highest possible - it seems this is
    // needed to get the capture device to assign the maximum possible frame
    // size for the buffers.
    {
        const resolution_s maxCaptureResolution = this->maximum_resolution();

        v4l2_format format;
        memset(&format, 0, sizeof(format));

        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!this->device_ioctl(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }

        format.fmt.pix.width = maxCaptureResolution.w;
        format.fmt.pix.height = maxCaptureResolution.h;
        format.fmt.pix.pixelformat = this->vcs_pixel_format_to_v4l_pixel_format(this->captureStatus.pixelFormat);
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!this->device_ioctl(VIDIOC_S_FMT, &format) ||
            !this->device_ioctl(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to set the current capture format for enqueuing capture buffers(error %d).", errno));
            goto fail;
        }

        if ((format.fmt.pix.width != maxCaptureResolution.w) ||
            (format.fmt.pix.height != maxCaptureResolution.h) ||
            (format.fmt.pix.pixelformat != this->vcs_pixel_format_to_v4l_pixel_format(this->captureStatus.pixelFormat)))
        {
            NBENE(("Failed to initialize the current capture format for enqueuing capture buffers (error %d).", errno));
            goto fail;
        }
    }

    // Tell the capture device that we've allocated frame buffers for it to use.
    {
        v4l2_requestbuffers buf;
        memset(&buf, 0, sizeof(buf));

        buf.count = this->backBuffer->numPages;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (!this->device_ioctl(VIDIOC_REQBUFS, &buf))
        {
            NBENE(("User pointer streaming couldn't be initialized (error %d).", errno));
            goto fail;
        }
    }

    // Enqueue the frame buffers we've allocated.
    for (unsigned i = 0; i < this->backBuffer->numPages; i++)
    {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)this->backBuffer->page(i).ptr();
        buf.length = this->backBuffer->page(i).size();

        if (!this->device_ioctl(VIDIOC_QBUF, &buf))
        {
            NBENE(("Failed to enqueue capture buffers (failed on buffer #%d).", (i + 1)));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

bool input_channel_v4l_c::start_capturing()
{
    if (!this->open_device(this->v4lDeviceFileName))
    {
        NBENE(("Failed to open the capture device '%s'.",
               this->v4lDeviceFileName.c_str()));

        goto fail;
    }

    // Verify device capabilities.
    {
        v4l2_capability caps = {};

        if (!this->device_ioctl(VIDIOC_QUERYCAP, &caps))
        {
            NBENE(("Failed to query the capabilities of capture device '%s'.",
                   this->v4lDeviceFileName.c_str()));

            goto fail;
        }

        if (!(caps.capabilities & V4L2_CAP_READWRITE))
        {
            NBENE(("The capture device '%s' doesn't support read/write access.",
                   this->v4lDeviceFileName.c_str()));

            goto fail;
        }
    }

    if (!this->enqueue_back_buffers())
    {
        NBENE(("Failed to enqueue back buffers for capturing."));

        goto fail;
    }

    // Set the starting capture resolution.
    {
        const resolution_s sourceResolution = this->source_resolution();

        v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!this->device_ioctl(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        format.fmt.pix.width = sourceResolution.w;
        format.fmt.pix.height = sourceResolution.h;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
        format.fmt.pix.field = V4L2_FIELD_NONE;

        if (!this->device_ioctl(VIDIOC_S_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        if (!this->device_ioctl(VIDIOC_G_FMT, &format))
        {
            NBENE(("Failed to query the current capture format (error %d).", errno));
            goto fail;
        }

        if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB32)
        {
            NBENE(("Failed to initialize the correct capture pixel format.", errno));
            goto fail;
        }

        this->captureStatus.resolution.w = format.fmt.pix.width;
        this->captureStatus.resolution.h = format.fmt.pix.height;
        this->captureStatus.resolution.bpp = 32;
    }

    // Start the capture stream.
    {
        v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!this->device_ioctl(VIDIOC_STREAMON, &bufType))
        {
            NBENE(("Couldn't start the capture stream for device '%s'.",
                   this->v4lDeviceFileName.c_str()));

            goto fail;
        }

        this->run = true;
        this->captureThreadFuture = std::async(std::launch::async,
                                               &input_channel_v4l_c::capture_thread,
                                               this);
    }

    if (!this->captureThreadFuture.valid())
    {
        goto fail;
    }

    this->push_capture_event(capture_event_e::new_video_mode);

    return true;

    fail:
    this->captureStatus.invalidDevice = true;
    this->run = false;
    this->reset_capture_event_flags();
    this->push_capture_event(capture_event_e::signal_lost);
    return false;
}

void input_channel_v4l_c::reset_capture_event_flags(void)
{
    for (auto &flag: this->captureEventFlags)
    {
        flag = 0;
    }

    return;
}

int input_channel_v4l_c::stop_capturing(void)
{
    // Ask the capture thread to stop, and block until it does so.
    this->run = false;
    const int retVal = (this->captureThreadFuture.valid()? this->captureThreadFuture.get() : 0);
  
    this->reset_capture_event_flags();
    this->push_capture_event(capture_event_e::signal_lost);

    // Stop the capture stream. Note that we don't return on error, we just
    // continue to force the capturing to stop.
    {
        v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (!this->device_ioctl(VIDIOC_STREAMOFF, &bufType))
        {
            DEBUG(("Couldn't stop the capture stream for device '%s'.",
                   this->v4lDeviceFileName.c_str()));
        }
    }

    // Note: we don't return on error, we just continue to force the capturing
    // to stop.
    if (!this->close_device())
    {
        DEBUG(("The capture device '%s' was not correctly closed.",
               this->v4lDeviceFileName.c_str()));
    }

    return retVal;
}

#endif
