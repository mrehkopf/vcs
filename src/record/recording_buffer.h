/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef RECORDING_BUFFERS_H
#define RECORDING_BUFFERS_H

#include <vector>
#include <queue>
#include <mutex>
#include "common/globals.h"
#include "common/memory/memory.h"
#include "common/memory/memory_interface.h"

// The recording buffer provides pre-allocated memory to hold frame pixel data.
// The buffer operates as a 'last in, first out' (LIFO) queue and is intended
// to be used by VCS's video recorder subsystem.
//
// Usage:
//
//   1. Initialize the buffer by calling initialize().
//
//   2. Add frames to the buffer by calling push() and copying the frame data to
//      the memory area identified by the returned pointer. This memory area will
//      be reserved - and thus won't be returned by subsequent calls to push() -
//      until the frame is removed from the buffer by calling pop().
//
//      Warning: Do not call push() if full() returns true.
//
//   3. Pop frames out of the buffer by calling pop(). Frames will be popped in
//      the order they were push()ed but in reverse (first in, last out).
//
//      Warning: Do not call pop() if empty() returns true.
//
//   4. When you no longer need the buffer, release it by calling release().
//
struct recording_buffer_s
{
    // Frames larger than this can't be stored in the buffer.
    const unsigned maxWidth = 2048;
    const unsigned maxHeight = 2048;

    // Allocate memory for the buffer, etc.
    void initialize(const size_t frameCapacity = 10);

    // Deallocate the buffer's memory, etc.
    void release(void);

    // Resets the buffer to an empty initial state.
    void reset(void);

    // Add a new frame into the buffer. Returns a pointer to its memory.
    heap_bytes_s<u8>* push(void);

    // Remove a frame from the buffer (last in, first out). Returns a pointer
    // to its memory.
    heap_bytes_s<u8>* pop(void);

    // Returns true if the buffer's maximum frame capacity is currently in use.
    // If so, no new frames can be push()ed until at least one is pop()ed.
    bool full(void);

    // Returns true if the buffer is currently holding no frames' data.
    bool empty(void);

    // Returns as a percentage the fraction of buffer capacity currently being
    // used.
    unsigned usage(void);

    // Memory for any temporary frame operations you might need to do. Feel free
    // to use it.
    heap_bytes_s<u8> scratchBuffer;

    // The VCS video recorder subsystem uses the buffer across threads, so this
    // mutex is provided for convenience.
    std::mutex mutex;

private:
    // Memory for the individual frames stored in the buffer.
    std::vector<heap_bytes_s<u8>> frameData;

    // For each 'frameData' element, a boolean indicating whether that element
    // is currently hosting a frame (and thus whether we're allowed to use
    // that element when push() is called).
    std::vector<bool> isFrameInUse;

    // A LIFO queue keeping track of the buffer's push()/pop() operations. The
    // back of the queue points to the most recently added frame, while the front
    // points to the least recently added frame.
    std::queue<size_t /*idx to 'frameData'*/> frameQueue;
};

#endif
