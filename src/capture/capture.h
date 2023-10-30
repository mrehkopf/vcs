/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

/*
 * The capture subsystem interface.
 *
 * The capture subsystem is responsible for mediating exchange between VCS and
 * the capture device; including initializing the device and providing VCS with
 * access to the contents of the frame buffer associated with the device.
 * 
 * An implementation of the capture subsystem will typically have a monitoring
 * thread that waits for the capture device to send in data. When data comes in,
 * the thread will copy it to a local memory buffer to be operated on in the
 * main VCS thread when it's ready to do so.
 *
 * ## Usage
 *
 * 1. Call kc_initialize_capture() to initialize the capture subsystem.
 *
 * 2. Use the interface functions to interact with the subsystem, making sure
 *    to observe the capture mutex (kc_mutex()). For example, kc_frame_buffer()
 *    returns the most recent captured frame's data.
 *
 * 3. VCS will automatically release the subsystem on program termination.
 *
 */

#ifndef VCS_CAPTURE_CAPTURE_H
#define VCS_CAPTURE_CAPTURE_H

#include <unordered_map>
#include <chrono>
#include <vector>
#include <mutex>
#include <any>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/refresh_rate.h"
#include "common/vcs_event/vcs_event.h"
#include "main.h"

#define LOCK_CAPTURE_MUTEX_IN_SCOPE std::lock_guard<std::mutex> lock(kc_mutex())

struct video_mode_s;

// VCS will periodically query the capture subsystem for the latest capture events.
// This enumerates the range of capture events that the capture subsystem can report
// back.
enum class capture_event_e
{
    // No capture events to report. Although some capture events may have occurred,
    // the capture subsystem chooses to not inform VCS of them.
    none,

    // Same as 'none', but the capture subsystem also thinks VCS shouldn't send
    // a new query for capture events for some short while (milliseconds); e.g.
    // because the capture device is currently not receiving a signal and so isn't
    // expected to produce events in the immediate future.
    sleep,

    // The capture device has just lost its input signal.
    signal_lost,

    // The capture device has just gained an input signal.
    signal_gained,

    // The capture device has sent in a new frame, whose data can be queried via
    // get_frame_buffer().
    new_frame,

    // The capture device's input signal has changed in resolution or refresh rate.
    new_video_mode,

    // The capture device's current input signal is invalid (e.g. out of range).
    invalid_signal,

    // The capture device isn't available for use.
    invalid_device,

    // An error has occurred with the capture device from which the capture
    // subsystem can't recover.
    unrecoverable_error,

    // Total enumerator count. Should remain the last item in the list.
    num_enumerators
};

struct video_mode_s
{
    resolution_s resolution;
    refresh_rate_s refreshRate;
};

enum class signal_format_e
{
    none = 0,
    analog,
    digital,
};

struct capture_rate_s
{
    static refresh_rate_s from_capture_device_properties(void);
    static void to_capture_device_properties(const refresh_rate_s &rate);
};

struct captured_frame_s
{
    std::chrono::time_point<std::chrono::steady_clock> timestamp = std::chrono::steady_clock::now();
    resolution_s resolution;
    uint8_t *pixels;
};

// Returns a reference to a mutex which a caller should acquire before interacting
// with the capture subsystem interface and which the capture subsystem should
// acquire before mutating data which the interface exposes.
//
// By default, code running in VCS's main loop (see @ref src/main.cpp) will be in
// scope of a lock on this mutex and so doesn't need to explicitly lock it.
std::mutex& kc_mutex(void);

// Initializes the capture subsystem.
//
// Returns a function that performs cleanup and release of the capture subsystem.
subsystem_releaser_t kc_initialize_capture(void);

// Prepares the capture device for use, including starting the capture stream.
//
// Throws on failure.
void kc_initialize_device(void);

// Releases the capture device, including stopping the capture stream and freeing
// any on-device buffers that were in use for the capture subsystem.
//
// Returns true on success; false otherwise.
bool kc_release_device(void);

// Returns the count of frames the capture subsystem has received from the
// capture device that VCS was too busy to process and display. In other words,
// a count of dropped frames.
//
// If this value gets above 0, it indicates that VCS is failing to process and
// display captured frames as fast as the capture device is producing them.
unsigned kc_dropped_frames_count(void);

// Returns true if the capture device's active input channel is currently
// receiving a signal; false otherwise.
bool kc_has_signal(void);

const std::vector<const char*>& kc_supported_video_preset_properties(void);

// Returns a reference to the most recent captured frame.
const captured_frame_s& kc_frame_buffer(void);

// Asks the capture subsystem to process the most recent or most important capture
// event.
capture_event_e kc_process_next_capture_event(void);

intptr_t kc_device_property(const std::string &key);

bool kc_set_device_property(const std::string &key, intptr_t value);

#endif
