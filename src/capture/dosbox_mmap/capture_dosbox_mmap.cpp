/*
 * 2021-2022 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Captures DOSBox's graphical output in Linux via a shared memory interface.
 *
 * Note: DOSBox must be modified using the provided patch to enable two-way
 * communication via the memory interface.
 *
 * Note: DOSBox must be set to render using OpenGL.
 *
 */

#include <atomic>
#include <future>
#include <cstring>
#include "common/globals.h"
#include "capture/capture.h"

// For shared memory.
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static bool IS_VALID_SIGNAL = true;

static captured_frame_s FRAME_BUFFER;

static const char MMAP_STATUS_BUF_FILENAME[] = "vcs_dosbox_mmap_status";
static const char MMAP_SCREEN_BUF_FILENAME[] = "vcs_dosbox_mmap_screen";
static const unsigned MMAP_STATUS_BUF_SIZE = 256;
static const unsigned MMAP_SCREEN_BUF_SIZE = MAX_NUM_BYTES_IN_CAPTURED_FRAME;
static uint8_t *MMAP_STATUS_BUF = nullptr;
static uint8_t *MMAP_SCREEN_BUF = nullptr;

static std::atomic<bool> RUN_CAPTURE_THREAD = {false};
static std::future<int> CAPTURE_THREAD_FUTURE;

static std::unordered_map<std::string, double> DEVICE_PROPERTIES = {
    {"width", 640},
    {"height", 480},

    {"width: minimum", MIN_CAPTURE_WIDTH},
    {"height: minimum", MIN_CAPTURE_HEIGHT},

    {"width: maximum", MAX_CAPTURE_WIDTH},
    {"height: maximum", MAX_CAPTURE_HEIGHT},

    {"has signal", true},
};

enum class screen_buffer_value_e : unsigned
{
    // Horizontal resolution of the frame whose data is currently in the buffer.
    width,

    // Vertical resolution of the frame whose data is currently in the buffer.
    height,

    // The screen buffer's pixels (BGRA/8888).
    pixels_ptr,
};

enum class status_buffer_value_e : unsigned
{
    // Set to 1 by DOSBox whenever it's finished copying a new frame into the
    // shared screen buffer; and to 0 by VCS whenever it finishes fetching a
    // frame from the shared screen buffer.
    is_new_frame_available,
};

static intptr_t get_screen_buffer_value(const screen_buffer_value_e valueEnum)
{
    k_assert(MMAP_SCREEN_BUF, "Attempting to access the screen buffer before its initialization.");

    switch (valueEnum)
    {
        case screen_buffer_value_e::width:
        {
            return *((uint16_t*)(&MMAP_SCREEN_BUF[0]));
        }
        case screen_buffer_value_e::height:
        {
            return *((uint16_t*)(&MMAP_SCREEN_BUF[2]));
        }
        case screen_buffer_value_e::pixels_ptr:
        {
            return (intptr_t)&MMAP_SCREEN_BUF[4];
        }
        default: k_assert(0, "Unrecognized enum value for querying the screen buffer.");
    }

    // This is expected to never be reached - prior to this, either a proper value
    // has been returned or an assertion thrown.
    return 0;
}

static intptr_t get_status_buffer_value(const status_buffer_value_e valueEnum)
{
    k_assert(MMAP_STATUS_BUF, "Attempting to access the status buffer before its initialization.");

    switch (valueEnum)
    {
        case status_buffer_value_e::is_new_frame_available:
        {
            return bool(MMAP_STATUS_BUF[0]);
        }
        default: k_assert(0, "Unrecognized enum value for querying the status buffer.");
    }

    // This is expected to never be reached - prior to this, either a proper value
    // has been returned or an assertion thrown.
    return 0;
}

static void set_status_buffer_value(const status_buffer_value_e valueEnum, const intptr_t value)
{
    k_assert(MMAP_STATUS_BUF, "Attempting to access the status buffer before its initialization.");

    switch (valueEnum)
    {
        case status_buffer_value_e::is_new_frame_available:
        {
            MMAP_STATUS_BUF[0] = value;
            break;
        }
        default: k_assert(0, "Unrecognized enum value for querying the status buffer.");
    }

    return;
}

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
static int capture_thread(void)
{
    while (RUN_CAPTURE_THREAD)
    {
        if (get_status_buffer_value(status_buffer_value_e::is_new_frame_available))
        {
            LOCK_CAPTURE_MUTEX_IN_SCOPE;

            IS_VALID_SIGNAL = true;
            
            const unsigned frameWidth = get_screen_buffer_value(screen_buffer_value_e::width);
            const unsigned frameHeight = get_screen_buffer_value(screen_buffer_value_e::height);

            if ((frameWidth > MAX_CAPTURE_WIDTH) ||
                (frameHeight > MAX_CAPTURE_HEIGHT) ||
                (frameWidth < MIN_CAPTURE_WIDTH) ||
                (frameHeight < MIN_CAPTURE_HEIGHT))
            {
                IS_VALID_SIGNAL = false;
                push_capture_event(capture_event_e::invalid_signal);
                continue;
            }

            if ((frameWidth != FRAME_BUFFER.resolution.w) ||
                (frameHeight != FRAME_BUFFER.resolution.h))
            {
                resolution_s::to_capture_device_properties({.w = frameWidth, .h = frameHeight});
                push_capture_event(capture_event_e::new_video_mode);
            }

            memcpy(
                FRAME_BUFFER.pixels,
                (char*)get_screen_buffer_value(screen_buffer_value_e::pixels_ptr),
                (frameWidth * frameHeight * 4)
            );

            push_capture_event(capture_event_e::new_frame);

            set_status_buffer_value(status_buffer_value_e::is_new_frame_available, false);
        }
    }

    return 1;
}

double kc_device_property(const std::string &key)
{
    return (DEVICE_PROPERTIES.contains(key)? DEVICE_PROPERTIES.at(key) : 0);
}

bool kc_set_device_property(const std::string &key, double value)
{
    if (key == "width")
    {
        if (
            (value > MAX_CAPTURE_WIDTH) ||
            (value < MIN_CAPTURE_WIDTH)
        ){
            return false;
        }

        FRAME_BUFFER.resolution.w = value;
    }
    else if (key == "height")
    {
        if (
            (value > MAX_CAPTURE_HEIGHT) ||
            (value < MIN_CAPTURE_HEIGHT)
        ){
            return false;
        }

        FRAME_BUFFER.resolution.h = value;
    }

    DEVICE_PROPERTIES[key] = value;

    return true;
}

void kc_initialize_device(void)
{
    DEBUG(("Initializing the virtual capture device."));
    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");

    FRAME_BUFFER.resolution = {.w = 640, .h = 480};
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    kc_set_device_property("width", FRAME_BUFFER.resolution.w);
    kc_set_device_property("height", FRAME_BUFFER.resolution.h);

    ev_new_video_mode.listen([](const video_mode_s &mode)
    {
        resolution_s::to_capture_device_properties(mode.resolution);
        refresh_rate_s::to_capture_device_properties(mode.refreshRate);
    });

    // Initialize the shared memory interface.
    {
        int fd = shm_open(MMAP_STATUS_BUF_FILENAME, (O_RDWR | O_CREAT), 0666);
        k_assert((fd >= 0), "Failed to create the shared memory file (status).");
        int ftrerr = ftruncate(fd, MMAP_STATUS_BUF_SIZE);
        k_assert((ftrerr == 0), "Failed to initialize the shared memory file (status).");
        MMAP_STATUS_BUF = (uint8_t*)mmap(nullptr, MMAP_STATUS_BUF_SIZE, 0666, MAP_SHARED, fd, 0);
        k_assert(MMAP_STATUS_BUF, "Failed to MMAP into the shared memory file (status).");

        fd = shm_open(MMAP_SCREEN_BUF_FILENAME, (O_RDWR | O_CREAT), 0666);
        k_assert((fd >= 0), "Failed to create the shared memory file (screen).");
        ftrerr = ftruncate(fd, MMAP_SCREEN_BUF_SIZE);
        k_assert((ftrerr == 0), "Failed to initialize the shared memory file (screen).");
        MMAP_SCREEN_BUF = (uint8_t*)mmap(nullptr, MMAP_SCREEN_BUF_SIZE, 0666, MAP_SHARED, fd, 0);
        k_assert(MMAP_SCREEN_BUF, "Failed to MMAP into the shared memory file (screen).");
    }

    // Start the capture thread.
    {
        RUN_CAPTURE_THREAD = true;
        CAPTURE_THREAD_FUTURE = std::async(std::launch::async, capture_thread);
        k_assert(CAPTURE_THREAD_FUTURE.valid(), "Failed to start the capture thread.");
    }

    return;
}

bool kc_release_device(void)
{
    delete [] FRAME_BUFFER.pixels;
    
    RUN_CAPTURE_THREAD = false;
    CAPTURE_THREAD_FUTURE.wait();

    return true;
}

const captured_frame_s& kc_frame_buffer(void)
{
    return FRAME_BUFFER;
}

capture_event_e kc_process_next_capture_event(void)
{
    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        ev_invalid_capture_signal.fire();
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        ev_new_proposed_video_mode.fire(video_mode_s{
            .resolution = FRAME_BUFFER.resolution
        });
        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        ev_new_captured_frame.fire(FRAME_BUFFER);
        return capture_event_e::new_frame;
    }

    return capture_event_e::none;
}

uint kc_dropped_frames_count(void)
{
    // Not supported.

    return 0;
}
