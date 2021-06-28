/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The memory subsystem interface.
 * 
 * The memory subsystem provides chunks of pre-allocated heap memory to the rest
 * of VCS and its subsystems for storing blocks of POD data.
 * 
 * The subsystem allocates a fixed-size memory buffer on initialization and deals
 * out portions of it as virtual allocations to callers. The subsystem also
 * maintains internal bookkeeping of requested allocations, allowing for monitoring
 * of VCS's memory usage.
 * 
 * @note
 * A C++-style wrapper for this interface is provided by heap_mem.
 * 
 * @warning
 * This memory interface is for POD data only; i.e. anything you wouldn't mind
 * using @a memset on.
 * 
 * ## Usage
 * 
 *   1. Call kmem_allocate() to request an allocation of memory:
 *      @code
 *      // Note: The allocator will 0-initialize the memory.
 *      char *bytes = (char*)kmem_allocate(101, "101 bytes");
 *      int *ints = (int*)kmem_allocate(2*sizeof(int), "A pair of ints");
 * 
 *      // Use the allocated memory as you would a standard allocation.
 *      bytes[0] = 82;
 *      ints[1] = 1234;
 *      @endcode
 * 
 *   2. You can use kmem_sizeof_allocation() to find the size of an allocation:
 *      @code
 *      char *buffer = (char*)kmem_allocate(44);
 *      unsigned size = kmem_sizeof_allocation((void*)buffer);
 *      // size == 44.
 *      @endcode
 * 
 *   3. Call kmem_release() to release an allocation back into circulation for
 *      subsequent calls to kmem_allocate():
 *      @code
 *      char *buffer = (char*)kmem_allocate(1);
 *      // buffer != NULL.
 * 
 *      kmem_release((void**)&buffer);
 *      // buffer == NULL.
 *      @endcode
 */

#ifndef VCS_COMMON_MEMORY_MEMORY_H
#define VCS_COMMON_MEMORY_MEMORY_H

#include <string>
#include "common/types.h"

/*!
 * Allocates a consecutive block of @p numBytes bytes and returns a pointer to
 * the first byte. The block's bytes are 0-initialized.
 * 
 * The allocation is virtual in that no new memory is allocated; rather, the
 * pointer returned is to an unused region of the memory subsystem's pre-allocated
 * fixed-size memory buffer.
 * 
 * For bookkeeping and debugging purposes, @p reason gives a string briefly
 * explaining the caller's intended purpose for the allocation (e.g. "Scratch
 * buffer").
 * 
 * Triggers an assertion failure if the allocation fails (e.g. if the memory
 * subsystem's fixed-size buffer doesn't have enough room for the requested
 * allocation).
 * 
 * @code
 * // Allocate 101 bytes.
 * char *buffer = (char*)kmem_allocate(101, "An example allocation");
 * @endcode
 * 
 * @note
 * The first runtime call to this function will cause the memory subsystem to
 * initialize its fixed-size memory buffer. Any unrecoverable failure in this
 * initialization (e.g. insufficient system memory) will result in an assertion
 * failure or other such error condition.
 * 
 * @see
 * kmem_release(), kmem_sizeof_allocation()
 */
void* kmem_allocate(const int numBytes, const char *const reason);

/*!
 * Releases an allocation made with kmem_allocate().
 * 
 * The allocation is identified by @p mem, which is the pointer returned by
 * kmem_allocate() for that allocation.
 * 
 * @note
 * @p mem will be set to NULL by the call. 
 * 
 * @code
 * // Make an allocation.
 * char *buffer = (char*)kmem_allocate(101, "An example allocation");
 * 
 * // buffer != NULL.
 * 
 * // Release the allocation.
 * kmem_release((void**)&buffer);
 * 
 * // buffer == NULL.
 * @endcode
 * 
 * @see
 * kmem_allocate()
 */
void kmem_release(void **mem);

/*!
 * Returns the size (in number of bytes) of an allocation made with kmem_allocate().
 * 
 * The allocation is identified by @p mem, which is the pointer returned by
 * kmem_allocate() for that allocation.
 * 
 * @code
 * char *buffer = (char*)kmem_allocate(101, "An example allocation");
 * 
 * unsigned size = kmem_sizeof_allocation((void*)buffer);
 * // size == 101.
 * @endcode
 * 
 * @see
 * kmem_allocate()
 */
uint kmem_sizeof_allocation(const void *const mem);

#endif
