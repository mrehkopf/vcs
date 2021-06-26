/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The interface to VCS's memory subsystem.
 * 
 * The memory subsystem provides pre-allocated heap memory to other VCS subsystems.
 * 
 * The subsystem allocates a fixed-size memory buffer on initialization and deals
 * out portions of the buffer as virtual allocations to callers.
 * 
 * The subsystem also maintains internal bookkeeping of requested allocations,
 * allowing for monitoring of VCS's memory usage.
 * 
 * @note
 * A C++-style wrapper interface is provided by @ref memory_interface.h.
 * 
 * ## Usage
 * 
 *   1. Call kmem_allocate() to request an allocation of memory:
 *      @code
 *      // Note: The memory is explicitly 0-initialized.
 *      char *bytes = (char*)kmem_allocate(101, "101 bytes");
 *      int *ints = (int*)kmem_allocate(2*sizeof(int), "A pair of ints");
 *      @endcode
 * 
 *   2. Use the memory as you would normally:
 *      @code
 *      bytes[0] = 82;
 *      ints[1] = 1234;
 *      @endcode
 * 
 *   3. You can use kmem_sizeof_allocation() to find the size of an allocation:
 *      @code
 *      unsigned size = kmem_sizeof_allocation(bytes);
 *      // size == 101.
 *      @endcode
 * 
 *   4. Call kmem_release() to release the memory buffer back into circulation
 *      for subsequent calls to kmem_allocate():
 *      @code
 *      // Note: The pointers are set to NULL by the call.
 *      kmem_release((void**)&bytes);
 *      kmem_release((void**)&ints);
 * 
 *      // bytes == NULL.
 *      // ints == NULL.
 *      @endcode
 *
 *   5. Call kmem_release_system() to release the memory subsystem.
 */

#ifndef VCS_COMMON_MEMORY_MEMORY_H
#define VCS_COMMON_MEMORY_MEMORY_H

#include <string>
#include "common/types.h"

/*!
 * Allocates a consecutive block of @p numBytes bytes and returns a pointer to
 * the first byte. The block's bytes are explicitly 0-initialized before
 * returning.
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
 * char *buffer = (char*)kmem_allocate(101, "101 bytes");
 * unsigned size = kmem_sizeof_allocation(buffer);
 * // size == 101.
 * @endcode
 * 
 * @see
 * kmem_allocate()
 */
uint kmem_sizeof_allocation(const void *const mem);

/*!
 * Releases the memory subsystem, deallocating its fixed-sized memory buffer.
 * 
 * @warning
 * This function should only be called on program exit, when PROGRAM_EXIT_REQUESTED
 * equals true.
 */
void kmem_release_system(void);

#include "memory_interface.h"

#endif
