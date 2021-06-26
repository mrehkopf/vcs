/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

/*! @file
 *
 * @brief
 * A frame buffer for VCS's video recording subsystem.
 * 
 */

#ifndef VCS_RECORD_RECORDING_BUFFER_H
#define VCS_RECORD_RECORDING_BUFFER_H

#include <vector>
#include <queue>
#include <mutex>
#include "common/globals.h"
#include "common/memory/memory.h"
#include "common/memory/memory_interface.h"

/*!
 * @brief
 * A memory buffer for storing captured frames, for use by VCS's video recording
 * subsystem.
 * 
 * This buffer allocates memory using VCS's memory subsystem for a given fixed
 * number of captured frames, and provides access to the memory via a LIFO-like
 * (last in, first out) queue interface.
 * 
 * The buffer provides the queue-like commands push() and pop(), of which push()
 * marks the next available frame slot in the buffer as being in use and returns
 * a pointer to its memory (which is big enough to hold one captured frame).
 * This memory is available to the caller until it's marked by a call to pop()
 * as no longer being in use, after which it's made available to subsequent push()
 * calls. In LIFO fashion, the slot reserved by the most recent call to push()
 * will be the last to be unreserved by pop().
 * 
 * Usage:
 *
 *   1. Initialize the buffer by calling initialize().
 * 
 *   2. To add a captured frame into the buffer, call push() to obtain a pointer
 *      to the next available frame slot, then copy the frame's pixel data into
 *      this memory area. The memory area is large enough to hold the maximum
 *      frame size supported by the buffer.
 *
 *   3. To retrieve and remove a frame from the buffer in LIFO fashion, call pop()
 *      and copy the frame pixel data from the memory to which the function returns
 *      a pointer.
 *
 *   4. When you no longer need the buffer, release it by calling release().
 */
struct recording_buffer_s
{
    /*! Specifies the maximum width of frame that the buffer can store.*/
    const unsigned maxWidth = 1024;

    /*! Specifies the maximum height of frame that the buffer can store.*/
    const unsigned maxHeight = 768;

    /*!
     * Initializes the buffer, allocating memory to hold at most the number of
     * frames given by @p frameCapacity.
     */
    void initialize(const size_t frameCapacity = 10);

    /*!
     * Releases the buffer.
     * 
     * @warning
     * This function deallocates the buffer's memory, invalidating all pointers
     * obtained from push() or pop().
     */
    void release(void);

    /*!
     * Resets the buffer to an empty state.
     */
    void reset(void);

    /*!
     * Reserves a frame slot in the buffer and returns a pointer to its memory.
     */
    heap_bytes_s<u8>* push(void);

    /*!
     * Unreserves a frame slot in the queue and returns a pointer to its memory.
     * 
     * The returned pointer will remain valid until either push() or release() is
     * called.
     */
    heap_bytes_s<u8>* pop(void);

    /*!
     * Returns true if the buffer is currently at maximum capacity; false
     * otherwise.
     * 
     * If the buffer is full, no new frames can be queued with push() until one
     * or more frame slots are unreserved with pop() or the buffer is reset with
     * reset().
     */
    bool full(void);

    /*!
     * Returns true if the buffer currently has no frame slots reserved; false
     * otherwise.
     */
    bool empty(void);

    /*!
     * Returns as a percentage (0--100) the amount of buffer capacity currently
     * in use.
     */
    unsigned usage(void);

    /*!
     * An extra memory buffer for any temporary frame operations the VCS video
     * recording subsystem might need to do.
     * 
     * The buffer has room for one captured frame.
     * 
     * @note
     * It's likely that this buffer will be removed in a future version of VCS,
     * since it's a bit of a temporary kludge.
     * 
     * @warning
     * This buffer's memory won't be allocated until initialize() is called.
     */
    heap_bytes_s<u8> scratchBuffer;

    /*!
     * A convenience mutex for use by VCS's video recorder subsystem.
     * 
     * Not used by the buffer.
     */
    std::mutex mutex;

private:
    /*!
     * Memory for the buffer's frame slots.
     * 
     * Calls to push() and pop() return pointers to this memory.
     */
    std::vector<heap_bytes_s<u8>> frameData;

    /*!
     * A flag for each frame slot in the buffer, indicating whether a given slot
     * is currently reserved (by a call to push()).
     */
    std::vector<bool> isFrameInUse;

    /*!
     * A LIFO queue for keeping track of the buffer's push()/pop() operations.
     */
    std::queue<size_t /*idx to 'frameData'*/> frameQueue;
};

#endif
