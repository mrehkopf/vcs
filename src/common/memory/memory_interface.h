/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * A C++ wrapper for the memory subsystem interface.
 */

#ifndef VCS_COMMON_MEMORY_MEMORY_INTERFACE_H
#define VCS_COMMON_MEMORY_MEMORY_INTERFACE_H

#include "common/globals.h"
#include "common/memory/memory.h"
#include "common/types.h"

/*!
 * @brief
 * A C++ wrapper for the memory subsystem interface (memory.h).
 * 
 * This wrapper provides a C++ API over the C-style memory subsystem interface,
 * as well as runtime bounds-checking.
 * 
 * The wrapper consists of an object with a templated C++ interface operating a
 * memory buffer acquired via the memory subsystem interface. Whereas the original
 * interface operates on raw bytes (returning void* pointers and requiring
 * explicit type-casting), this wrapper operates natively on built-in data types
 * (e.g. @a char and @a short) and internally translates them into/from the
 * original interface's plain byte arrays.
 * 
 * @warning
 * This memory interface is for POD data only; i.e. anything you wouldn't mind
 * using @a memset on.
 * 
 * @see
 * [Memory subsystem interface](@ref memory.h)
 * 
 * ## Usage
 * 
 *   1. Allocate memory for POD data types:
 *      @code
 *      // Allocate memory for 2 ints.
 *      heap_bytes_s<int> ints(2);
 * 
 *      // Create an object but delay its memory allocation.
 *      heap_bytes_s<double> doubles;
 *      doubles.alloc(11);
 * 
 *      // It's recommended that you provide a string describing your intended
 *      // purpose for the allocation. It may be used for descriptive debug
 *      // messages etc.
 *      heap_bytes_s<char> scratch(640*480, "Scratch buffer for color conversion");
 *      @endcode
 * 
 *   2. Use the allocated memory:
 *      @code
 *      heap_bytes_s<int> ints(2);
 *      heap_bytes_s<char> chars(10);
 * 
 *      // These memory accesses are bounds-checked at runtime if the RELEASE_BUILD
 *      // build flag isn't set; otherwise, no bounds-checking is done.
 *      ints[1] = 1234;
 *      chars[0] = 82;
 *      int a = ints[1]; // a == 1234.
 *      char b = chars[0]; // b == 82.
 * 
 *      // We only allocated memory for 2 ints, so this read overflows. It triggers a
 *      // bounds-checking assertion failure if the RELEASE_BUILD build flag isn't set.
 *      int c = ints[3];
 * 
 *      // The underlying memory pointer (of type T) is also available.
 *      int *exposedBuffer = ints.ptr();
 *      @endcode
 * 
 *   4. Release the allocated memory when you no longer need it:
 *      @code
 *      heap_bytes_s<char> chars(10);
 *      // chars.is_null() == false.
 * 
 *      chars.release_memory();
 *      // chars.is_null() == true.
 * 
 *      // Attempting to access null (in this case, released) memory. Triggers an
 *      // assertion failure if the RELEASE_BUILD build flag isn't set; otherwise
 *      // results in undefined behavior.
 *      chars[0] = 15;
 *      @endcode
 */
template <typename T>
struct heap_bytes_s
{
    /*!
     * Initializes the object without allocating memory for its data buffer. The
     * memory can be allocated later with alloc().
     * 
     * @code
     * heap_bytes_s<char> buffer;

     * // buffer.is_null() == true.
     * // buffer.size() == 0.
     * @endcode
     * 
     * @note
     * Attempting to dereference the object's unallocated data buffer (e.g. with
     * heap_bytes_s::operator[]) will result in a bounds-checking assertion failure
     * if the RELEASE_BUILD build flag isn't set, and undefined behavior otherwise.
     * 
     * @see
     * alloc()
     */
    heap_bytes_s(void)
    {
        this->data = nullptr;
        this->dataSize = 0;
    }

    /*!
     * Initializes the object and allocates @p num elements of type @p T for its
     * data buffer.
     * 
     * This function calls kmem_allocate() to perform the memory allocation. See
     * that function's documentation for more information about the memory it
     * allocates.
     * 
     * @code
     * heap_bytes_s<char> buffer(10);
     * 
     * // buffer.is_null() == false.
     * // buffer.size() == 10.
     * // buffer[0] == 0.
     * @endcode
     * 
     * @see
     * kmem_allocate()
     */
    heap_bytes_s(const uint num, const char *const reason = nullptr)
    {
        alloc(num, reason);

        return;
    }

    /*!
     * Returns a reference to the element at offset @p offs in the data buffer,
     * bounds-checking the offset unless the RELEASE_BUILD build flag is set.
     * 
     * If the bounds check fails, triggers an assertion failure.
     *
     * @code
     * // Allocate 10 ints.
     * heap_bytes_s<int> buffer(10);
     * 
     * // Set the value of the 2nd int in the data buffer. This access will
     * // be bounds-checked if the RELEASE_BUILD build flag isn't set.
     * buffer[1] = 0;
     * 
     * // An overflowing write that attempts to modify the 11th element when only
     * // 10 elements have been allocated. Triggers an assertion failure if the
     * // RELEASE_BUILD build flag isn't set.
     * buffer[10] = 0;
     * @endcode
     * 
     * @see
     * ptr()
     */
    T& operator[](const uint offs) const
    {
        k_assert_optional((this->data != nullptr), "Tried to access null heap memory.");
        k_assert_optional((offs < this->size()), "Accessing heap memory out of bounds.");

        return this->data[offs];
    }

    /*!
     * Returns a pointer (of type @a T) to the first byte in the data buffer.
     * 
     * @code
     * heap_bytes_s<int> buffer(10);
     * int *rawPointer = buffer.ptr();
     * 
     * // These refer to the same memory element, but the direct pointer access
     * // never does bounds-checking.
     * buffer[1];
     * rawPointer[1];
     * 
     * // You could use the pointer e.g. with the mem* functions.
     * std::memset(buffer.ptr(), 0, buffer.size());
     * @endcode
     * 
     * @see
     * heap_bytes_s::operator[]
     */
    T* ptr(void) const
    {
        k_assert_optional(this->data != nullptr, "Tried to access null heap memory.");

        return this->data;
    }

    /*!
     * Compares @p size against the size() of the data buffer. If @p size is larger,
     * triggers an assertion failure. Otherwise, returns @p size.
     * 
     * This function's intended use case is to act as a drop-in bounds-check for
     * e.g. the mem* functions.
     * 
     * @code
     * heap_bytes_s<char> buffer(1000);
     * 
     * unsigned x = 10;
     * unsigned y = 5;
     * 
     * // We want to memset x*y bytes of the data buffer. The up_to() function
     * // ensures that this value doesn't exceed the buffer's maximum capacity.
     * std::memset(buffer.ptr(), 0, buffer.up_to(x * y));
     * @endcode
     */
    uint up_to(const uint size) const
    {
        k_assert((size <= this->size()), "Possible memory access out of bounds.");

        return size;
    }

    /*!
     * Sets the data buffer pointer to point to the first byte of the data buffer
     * of the @p other memory object.
     * 
     * @warning
     * Releasing the data buffer of @p other doesn't update pointers that point to
     * it. Accessing the released memory via these pointers will result in
     * undefined behavior that bypasses runtime bounds checks.
     * 
     * @code
     * heap_bytes_s<int> ints(2);
     * 
     * heap_bytes_s<int> intsRef;
     * intsRef.point_to(ints);
     * 
     * ints[0] = 1234;
     * // intsRef[0] == 1234.
     * 
     * intsRef[0] = 1;
     * // ints[0] == 1.
     * 
     * ints.release_memory();
     * // ints.is_null() == true.
     * // intsRef.is_null() == false.
     * 
     * // Undefined behavior, because the source buffer was released. The runtime
     * // bounds-checking of operator[] doesn't work, because it still thinks the
     * // data buffer exists.
     * intsRef[0];
     * 
     * intsRef.release_memory();
     * // intsRef.is_null() == true.
     * 
     * // Will now correctly trigger the bounds-checking of operator[].
     * intsRef[0];
     * @endcode
     * 
     * @see
     * point_to(T *const, const int)
     */
    void point_to(heap_bytes_s<T> &other)
    {
        k_assert(!other.is_null(), "Can't point to a null memory object.");

        return this->point_to(other.ptr(), other.size());
    }

    /*!
     * Sets the data buffer pointer to point to @p newPtr, which is a memory region
     * of @p size bytes.
     * 
     * @code
     * int ints[10];
     * heap_bytes_s<int> intsRef;
     * 
     * intsRef.point_to(ints, (10 * sizeof(ints[0])));
     * 
     * ints[0] = 1234;
     * // intsRef[0] == 1234.
     * @endcode
     * 
     * @see
     * point_to(heap_bytes_s<T> &)
     */
    void point_to(T *const newPtr, const int size)
    {
        k_assert(this->data == nullptr, "Can't assign a pointer to a non-null memory object.");
        k_assert(size > 0, "Can't assign with data sizes less than 1.");

        this->data = newPtr;
        this->dataSize = size;
        this->alias = true;

        return;
    }

    /*!
     * Allocates memory for the data buffer.
     * 
     * @warning
     * Call release_memory() before re-allocating. Attempting to allocate over an
     * existing data buffer will trigger an assertion failure.
     * 
     * @code
     * // Allocate memory for 10 ints using the constructor.
     * heap_bytes_s<int> ints(10);
     * 
     * // Allocate memory for 10 ints using alloc().
     * heap_bytes_s<int> ints2;
     * ints2.alloc(10);
     * 
     * // Allocate memory for 10 ints, then re-allocate for only 5 ints.
     * {
     *    heap_bytes_s<int> ints3(10);
     *    // ints3.size() == 10.
     * 
     *    ints3.release_memory();
     *    // ints3.size() == 0.
     * 
     *    ints3.alloc(5);
     *    // ints3.size() == 5.
     * }
     * @endcode
     */
    void alloc(const int size, const char *const reason = nullptr)
    {
        k_assert(!data, "Attempting to doubly allocate.");

        this->data = (T*)kmem_allocate(sizeof(T) * size, (reason == nullptr? "(No reason given.)" : reason));
        this->dataSize = size;

        return;
    }

    /*!
     * Releases the memory allocated to the data buffer.
     * 
     * This function calls kmem_release() to release the memory. See that function's
     * documentation for more information about the process.
     * 
     * If called on a data buffer allocated with point_to(), will only mark this
     * memory object's data buffer as null without releasing the memory to which
     * it points.
     * 
     * @note
     * Requests to release a null data buffer will be ignored.
     * 
     * @code
     * heap_bytes_s<int> ints;
     * // ints.is_null() == true.
     * 
     * ints.alloc(1);
     * // ints.is_null() == false.
     * 
     * ints.release_memory();
     * // ints.is_null() == true.
     * 
     * // This call is ignored, because the buffer has already been released.
     * ints.release_memory();
     * // ints.is_null() == true.
     * @endcode
     * 
     * @code
     * heap_bytes_s<int> ints(2);
     * 
     * heap_bytes_s<int> intsRef;
     * intsRef.point_to(ints);
     * 
     * // Calling release_memory() on a data buffer allocated with point_to()
     * // only sets that memory object's data pointer to null without releasing
     * // the buffer.
     * intsRef.release_memory();
     * // intsRef.is_null() == true.
     * // ints.is_null() == false.
     * @endcode
     * 
     * @see
     * kmem_release()
     */
    void release_memory(void)
    {
        if (this->data == nullptr)
        {
            DEBUG(("Was asked to release a null pointer in the memory cache. Ignoring this."));
            return;
        }

        if (!this->alias)
        {
            kmem_release((void**)&this->data);
            this->dataSize = 0;
        }
        else
        {
            DEBUG(("Marking aliased memory to null instead of releasing it."));
            this->data = nullptr;
            this->dataSize = 0;
        }

        return;
    }

    /*!
     * Returns the number of elements (of type @p T) allocated to the data buffer.
     * 
     * @code
     * heap_bytes_s<int> ints(11);
     * // ints.size() == 11.
     * @endcode
     * 
     * @code
     * heap_bytes_s<int> ints;
     * // ints.size() == 0.
     * @endcode
     */
    uint size(void) const
    {
        return this->dataSize;
    }

    /*!
     * Returns true if the memory object currently has a null data buffer; false
     * otherwise.
     * 
     * @code
     * // The default constructor doesn't allocate the data buffer.
     * heap_bytes_s<int> ints;
     * // ints.is_null() == true.
     * 
     * ints.alloc(1);
     * // ints.is_null() == false.
     * 
     * ints.release_memory();
     * // ints.is_null() == true.
     * @endcode
     * 
     * @code
     * heap_bytes_s<int> ints(2);
     * 
     * heap_bytes_s<int> intsRef;
     * // intsRef.is_null() == true.
     * 
     * intsRef.point_to(ints);
     * // intsRef.is_null() == false.
     * @endcode
     * 
     * @see
     * alloc
     */
    bool is_null(void) const
    {
        return !bool(this->data);
    }

private:
    T *data = nullptr;
    uint dataSize = 0;
    bool alias = false; // Set to true if the data pointer was assigned from outside rather than allocated specifically for this memory object.
};

#endif
