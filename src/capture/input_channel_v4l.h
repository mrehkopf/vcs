/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 * 
 * An interface for communicating with a Video4Linux /dev/videoX device. Intended
 * to work as part of VCS's Video4Linux capture API implementation.
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

#ifndef VCS_CAPTURE_INPUT_CHANNEL_V4L_H
#define VCS_CAPTURE_INPUT_CHANNEL_V4L_H

#include <future>
#include <chrono>
#include <atomic>
#include <vector>
#include "common/globals.h"
#include "capture/capture_api.h"
#include "common/refresh_rate.h"
#include "capture/ic_v4l_video_parameters.h"

struct v4l2_format;

class input_channel_v4l_c
{
public:
    // Open the input channel (/dev/videoX device) and start capturing from
    // it.
    input_channel_v4l_c(capture_api_s *const parentCaptureAPI,
                        const std::string v4lDeviceFileName,
                        const unsigned numBackBuffers,
                        captured_frame_s *const dstFrameBuffer);

    ~input_channel_v4l_c();

    // Returns a copy of the current state of the given capture flag, then unsets
    // the flag.
    bool pop_capture_event(const capture_event_e flag);

    // Execute an ioctl() on this input channel's underlying /dev/videoX device.
    // Returns true on success; false otherwise (see errno for ioctl() errors).
    bool device_ioctl(const unsigned long request, void *data);

    // Returns true if the given format's parameters indicate a valid signal (e.g.
    // that the signal's resolution isn't out of range).
    static bool is_format_of_valid_signal(const v4l2_format *const format);

    // Metadata about the current capture on this input channel.
    struct
    {
        // True if this channel is currently not receiving a signal.
        bool noSignal = false;

        // True if the signal we're currently receiving is invalid in some way
        // (e.g. out of range).
        bool invalidSignal = false;

        // Set to true if we were unable to start capturing on the given capture
        // device.
        bool invalidDevice = false;

        // The current capture resolution.
        resolution_s resolution = {1024, 768, 32};

        refresh_rate_s refreshRate = refresh_rate_s(0);

        ic_v4l_video_parameters_c videoParameters;

        // Count of the frames we've captured that VCS has finished processed.
        std::atomic<unsigned int> numFramesProcessed = {0};

        std::atomic<unsigned int> numFramesCaptured = {0};
        
        // Count of frames we've captured which VCS wasn't able to process,
        // e.g. due to being busy processing a previous frame.
        std::atomic<unsigned int> numNewFrameEventsSkipped = {0};

        capture_pixel_format_e pixelFormat = capture_pixel_format_e::rgb_888;
    } captureStatus;

private:
    // Runs in its own thread; continuously polls for new capture events on
    // this input channel. Returns 1 if exited successfully; 0 otherwise
    // (see captureThreadFuture).
    int capture_thread(void);

    // Poll the capture devicve for a new frame. Sets capture events flags
    // accordingly. On success, returns true and either copies the new frame's
    // data to dstFrameBuffer or does nothing if no new frame was available. On
    // error, returns false.
    bool capture_thread__get_next_frame(void);

    // Launch the capture thread. Returns true on success; false otherwise.
    bool start_capturing(void);

    // End and close the capture thread. Returns the return value of capture_thread().
    int stop_capturing(void);

    // Open the given V4L /dev/videoX capture device. Returns true on success;
    // false otherwise.
    bool open_device(const std::string &deviceFileName);

    // Returns true on success; false otherwise.
    bool close_device(void);

    // Returns true if the current video mode (resolution, refresh rate, etc.) has
    // changed; false otherwise.
    bool capture_thread__has_source_mode_changed(void);

    // Mark the given capture event as having occurred.
    void push_capture_event(capture_event_e event);

    // Sets the resolution of the Video4Linux capture buffers.
    bool set_v4l_buffer_resolution(const resolution_s &resolution);

    void reset_capture_event_flags(void);

    // Prepare the input channel's back buffers for capture.  Returns true on
    // success; false otherwise.
    bool enqueue_mmap_back_buffers(const resolution_s &resolution);
    bool dequeue_mmap_back_buffers(void);

    bool streamon(void);
    bool streamoff(void);

    // Metadata about each mmap() back buffer we've created.
    struct mmap_metadata
    {
        uint8_t *ptr;
        unsigned length;
    };
    std::vector<mmap_metadata> mmapBackBuffers;

    // The number of back buffers our parent capture API asked us to use. Note that
    // the capture device may not be able to supply this many.
    const unsigned requestedNumBackBuffers;

    // Returns the maximum supported capture resolution for this input channel.
    resolution_s maximum_resolution(void) const;

    // Returns the resolution of the current signal on this input channel.
    resolution_s source_resolution(void);

    // Converts VCS's pixel format enumerator into Video4Linux's pixel format
    // identifier.
    u32 vcs_pixel_format_to_v4l_pixel_format(capture_pixel_format_e fmt) const;

    // Flags that the capture channel will set to convey to the VCS thread the
    // various capture events it detects during capture.
    std::vector<unsigned> captureEventFlags = std::vector<unsigned>(static_cast<unsigned>(capture_event_e::num_enumerators));

    // The VCS capture API that this input channel was created by.
    capture_api_s *const captureAPI;

    std::string v4lDeviceFileName = "";

    // The value returned by open(deviceFileName).
    int v4lDeviceFileHandle = -1;

    // The frame buffer we'll output captured frames into. Expected to be hosted
    // by the parent capture API
    captured_frame_s *const dstFrameBuffer;

    // A future holding the return value of capture_thread().
    std::future<int> captureThreadFuture;

    // Will be set to true before launching capture_thread(). The thread will
    // run until this is set to false.
    std::atomic<bool> run = {false};
};

#endif

#endif
