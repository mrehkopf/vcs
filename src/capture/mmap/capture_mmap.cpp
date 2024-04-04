/*
 * 2021-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * Implements an MMAP capture backend for shared memory communication between
 * VCS and another application, for the application to share its frame buffer
 * with VCS.
 *
 * The interface consists of two shared buffers: STATUS and SCREEN. The SCREEN
 * buffer contains the pixels of the application's frame buffer, and the STATUS
 * buffer various means of communication between the application and VCS.
 *
 * The byte-level layout of the status buffer:
 *
 *   0:  uint16_t  The width of the image currently in the screen buffer. Set by the application.
 *   2:  uint16_t  The height of the image currently in the screen buffer. Set by the application.
 *   4:  uint16_t  The maximum width the SCREEN buffer can hold. Set by VCS.
 *   6:  uint16_t  The maximum height the SCREEN buffer can hold. Set by VCS.
 *   8:  uint16_t  Capture flag. Set to 1 by the application when it has made a new frame available via the SCREEN buffer; set to 0 by VCS when it has received the frame. The application shouldn't modify the SCREEN buffer while this has a value of 1.
 *   10: uint16_t  Number of frames the application attempted to share with VCS while the value of the capture flag was 1. This is effectively a count of dropped frames.
 *
 * How the application should use the interface (assuming STATUS is accessed as
 * an array of uint16_t):
 *
 *   1. Acquire the two shared buffer files, "vcs_mmap_status" and
 *      "vcs_mmap_screen", using shm_open() and mmap().
 *
 *   2. On each render loop, check whether STATUS[4] is 0. If it isn't, increment
 *      a local numFramesDropped variable and move on - this represents a dropped
 *      frame. Otherwise, copy the frame buffer to SCREEN, and set STATUS[5] to
 *      numFramesDropped, STATUS[0] and STATUS[1] to the width and height of the
 *      frame buffer, and STATUS[4] to 1.
 *
 */

#include <atomic>
#include <future>
#include <chrono>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "common/globals.h"
#include "capture/capture.h"

namespace status
{
    enum
    {
        width,
        height,
        max_width,
        max_height,
        new_frame_available,
        dropped_frames_count,
    };
}

static struct shared_buffer
{
    std::string filename;
    unsigned size;
    uint16_t *_data = nullptr;

    void acquire(void)
    {
        const int fd = shm_open(this->filename.c_str(), (O_RDWR | O_CREAT), 0666);
        k_assert((fd >= 0), "Failed to create the shared memory file.");
        const int ftrerr = ftruncate(fd, this->size);
        k_assert((ftrerr == 0), "Failed to initialize the shared memory file.");
        this->_data = (uint16_t*)mmap(nullptr, this->size, 0666, MAP_SHARED, fd, 0);
        k_assert(this->_data, "Failed to MMAP into the shared memory file.");
    }

    uint16_t* data(void) const
    {
        k_assert(this->_data, "Attempting to access buffer data before it has been acquired.");
        return this->_data;
    }

    uintptr_t operator()(const unsigned valueEnum)
    {
        return this->data()[valueEnum];
    }

    uint16_t& operator[](const unsigned valueEnum)
    {
        return this->data()[valueEnum];
    }
}
MMAP_STATUS_BUFFER = {
    .filename = "vcs_mmap_status",
    .size = 256,
},
MMAP_SCREEN_BUFFER = {
    .filename = "vcs_mmap_screen",
    .size = MAX_NUM_BYTES_IN_CAPTURED_FRAME,
};

static std::atomic<bool> RUN_CAPTURE_LOOP = {false};
static std::future<int> CAPTURE_THREAD;
static unsigned NUM_DROPPED_FRAMES = 0;
static bool IS_VALID_SIGNAL = true;
static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];
static captured_frame_s FRAME_BUFFER = {
    .resolution = {.w = 640, .h = 480},
    .pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]()
};

static std::unordered_map<std::string, intptr_t> DEVICE_PROPERTIES = {
    {"api name", intptr_t("MMAP")},
    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},
    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},
    {"has signal", true},
};

static void push_capture_event(const capture_event_e event)
{
    CAPTURE_EVENT_FLAGS[(int)event] = true;
    return;
}

static bool pop_capture_event(const capture_event_e event)
{
    const bool oldFlagValue = CAPTURE_EVENT_FLAGS[(int)event];
    CAPTURE_EVENT_FLAGS[(int)event] = false;
    return oldFlagValue;
}

// Runs in its own thread, polling the memory shared with DOSBox for new frames.
// Returns 1 on successful exit; 0 otherwise.
static int capture_loop(void)
{
    while (RUN_CAPTURE_LOOP)
    {
        if (MMAP_STATUS_BUFFER(status::new_frame_available))
        {
            LOCK_CAPTURE_MUTEX_IN_SCOPE;
            
            const unsigned frameWidth = MMAP_STATUS_BUFFER(status::width);
            const unsigned frameHeight = MMAP_STATUS_BUFFER(status::height);

            if ((frameWidth > MAX_CAPTURE_WIDTH) ||
                (frameHeight > MAX_CAPTURE_HEIGHT) ||
                (frameWidth < MIN_CAPTURE_WIDTH) ||
                (frameHeight < MIN_CAPTURE_HEIGHT))
            {
                IS_VALID_SIGNAL = false;
                push_capture_event(capture_event_e::invalid_signal);
                goto done;
            }
            else
            {
                IS_VALID_SIGNAL = true;
            }

            if ((frameWidth != FRAME_BUFFER.resolution.w) ||
                (frameHeight != FRAME_BUFFER.resolution.h))
            {
                FRAME_BUFFER.resolution.w = frameWidth;
                FRAME_BUFFER.resolution.h = frameHeight;
                push_capture_event(capture_event_e::new_video_mode);
            }

            push_capture_event(capture_event_e::new_frame);
            FRAME_BUFFER.timestamp = std::chrono::steady_clock::now();
            memcpy(
                FRAME_BUFFER.pixels,
                (char*)MMAP_SCREEN_BUFFER.data(),
                (frameWidth * frameHeight * 4)
            );

            done:
            NUM_DROPPED_FRAMES += MMAP_STATUS_BUFFER[status::dropped_frames_count];
            MMAP_STATUS_BUFFER[status::new_frame_available] = false;
        }
    }

    return 1;
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the virtual capture device."));

    MMAP_STATUS_BUFFER.acquire();
    MMAP_SCREEN_BUFFER.acquire();
    resolution_s::to_capture_device_properties(FRAME_BUFFER.resolution);

    MMAP_STATUS_BUFFER[status::max_width] = kc_device_property("width: maximum");
    MMAP_STATUS_BUFFER[status::max_height] = kc_device_property("height: maximum");

    RUN_CAPTURE_LOOP = true;
    CAPTURE_THREAD = std::async(std::launch::async, capture_loop);
    k_assert(CAPTURE_THREAD.valid(), "Failed to start the capture thread.");

    ev_new_video_mode.listen([](const video_mode_s &mode)
    {
        resolution_s::to_capture_device_properties(mode.resolution);
        refresh_rate_s::to_capture_device_properties(mode.refreshRate);
        capture_rate_s::to_capture_device_properties(mode.refreshRate);
    });

    return;
}

capture_event_e kc_process_next_capture_event(void)
{
    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        ev_invalid_capture_signal.fire();
        return capture_event_e::invalid_signal;
    }

    if (pop_capture_event(capture_event_e::new_video_mode))
    {
        resolution_s::to_capture_device_properties(FRAME_BUFFER.resolution);
        ev_new_proposed_video_mode.fire(video_mode_s{
            .resolution = FRAME_BUFFER.resolution
        });
        return capture_event_e::new_video_mode;
    }

    if (pop_capture_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(FRAME_BUFFER);
        return capture_event_e::new_frame;
    }

    return capture_event_e::sleep;
}

intptr_t kc_device_property(const std::string &key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string &key, intptr_t value)
{
    DEVICE_PROPERTIES[key] = value;
    return true;
}

bool kc_release_device(void)
{
    RUN_CAPTURE_LOOP = false;
    CAPTURE_THREAD.wait();
    delete [] FRAME_BUFFER.pixels;
    return true;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return FRAME_BUFFER;
}

unsigned kc_dropped_frames_count(void)
{
    return NUM_DROPPED_FRAMES;
}
