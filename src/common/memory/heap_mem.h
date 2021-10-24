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
 * A C++ wrapper for the memory subsystem interface.
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
 * The memory interface is for POD data only; i.e. anything you wouldn't mind
 * using @a memset() on.
 * 
 * ## Usage
 * 
 *   1. Allocate memory for POD data types:
 *      @code
 *      // Allocate memory for 2 ints.
 *      heap_mem<int> ints(2);
 * 
 *      // Create an object but delay its memory allocation.
 *      heap_mem<double> doubles;
 *      doubles.allocate(11);
 * 
 *      // It's recommended that you provide a string describing your intended
 *      // purpose for the allocation. It may be used for descriptive debug
 *      // messages etc.
 *      heap_mem<char> scratch(640*480, "Scratch buffer for color conversion");
 *      @endcode
 * 
 *   2. Use the allocated memory:
 *      @code
 *      heap_mem<int> ints(2);
 *      heap_mem<char> chars(10);
 * 
 *      // These memory accesses are bounds-checked at runtime if the RELEASE_BUILD
 *      // build flag isn't defined; otherwise, no bounds-checking is done.
 *      ints[1] = 1234;
 *      chars[0] = 82;
 *      int a = ints[1]; // a == 1234.
 *      char b = chars[0]; // b == 82.
 * 
 *      // We allocated memory for 2 ints, so this overflows. It triggers a bounds-
 *      // checking assertion failure if the RELEASE_BUILD build flag isn't defined.
 *      int c = ints[9];
 * 
 *      // The underlying memory pointer (of type T) is also available.
 *      int *exposedBuffer = ints.data();
 *      @endcode
 * 
 *   4. Release the allocated memory when you no longer need it:
 *      @code
 *      heap_mem<char> chars(10);
 *      // chars.is_null() == false.
 * 
 *      chars.release();
 *      // chars.is_null() == true.
 * 
 *      // Attempting to access null (in this case, released) memory. Triggers an
 *      // assertion failure if the RELEASE_BUILD build flag isn't defined;
 *      // otherwise results in undefined behavior.
 *      chars[0] = 15;
 *      @endcode
 * 
 * @see
 * [the underlying memory subsystem interface](@ref memory.h)
 */
template <typename T>
class heap_mem
{
public:
    /*!
     * Initializes the object without allocating memory for its data buffer. The
     * memory can be allocated later with allocate().
     * 
     * @code
     * heap_mem<char> buffer;
     * // buffer.is_null() == true.
     * // buffer.count() == 0.
     * @endcode
     * 
     * @note
     * Attempting to dereference the object's unallocated data buffer (e.g. with
     * heap_mem::operator[]) will result in a bounds-checking assertion failure
     * if the RELEASE_BUILD build flag isn't defined, and undefined behavior
     * otherwise.
     * 
     * @see
     * allocate()
     */
    heap_mem(void)
    {
        this->data_ = nullptr;
        this->elementCount_ = 0;
    }

    /*!
     * Initializes the object and allocates @p numElements count of elements of
     * type @p T for its data buffer.
     * 
     * This function calls kmem_allocate() to perform the memory allocation. See
     * that function's documentation for more information about the memory it
     * allocates.
     * 
     * @code
     * heap_mem<char> buffer(10);
     * // buffer.is_null() == false.
     * // buffer.count() == 10.
     * // buffer[0] == 0.
     * @endcode
     * 
     * @see
     * kmem_allocate()
     */
    heap_mem(const unsigned numElements, const char *const reason = nullptr)
    {
        this->allocate(numElements, reason);

        return;
    }

    /*!
     * Returns a reference to the element at index @p idx in the data buffer,
     * bounds-checking the offset unless the RELEASE_BUILD build flag is set.
     * 
     * If the bounds check fails, triggers an assertion failure.
     *
     * @code
     * // Allocate 10 ints.
     * heap_mem<int> buffer(10);
     * 
     * // Set the value of the 2nd int in the data buffer. This access will
     * // be bounds-checked if the RELEASE_BUILD build flag isn't defined.
     * buffer[1] = 0;
     * 
     * // An overflowing write that attempts to modify the 11th element when only
     * // 10 elements have been allocated. Triggers an assertion failure if the
     * // RELEASE_BUILD build flag isn't defined.
     * buffer[10] = 0;
     * @endcode
     * 
     * @see
     * data()
     */
    T& operator[](const unsigned idx) const
    {
        k_assert_optional((this->data_ != nullptr), "Tried to access null heap memory.");
        k_assert_optional((idx < this->count()), "Accessing heap memory out of bounds.");

        return this->data_[idx];
    }

    /*!
     * Returns a pointer (of type @a T) to the first byte in the data buffer.
     * 
     * @code
     * heap_mem<int> buffer(10);
     * int *rawPointer = buffer.data();
     * 
     * // These refer to the same memory element, but the direct pointer access
     * // never does bounds-checking.
     * buffer[1];
     * rawPointer[1];
     * 
     * // You could use the pointer e.g. with the mem* functions.
     * std::memset(buffer.data(), 0, buffer.size());
     * @endcode
     * 
     * @note
     * If the RELEASE_BUILD build flag isn't defined, calling this function when
     * the data buffer is unallocated (is_null() returns true) will result in an
     * assertion failure.
     * 
     * @see
     * heap_mem::operator[]
     */
    T* data(void) const
    {
        k_assert_optional(this->data_, "Tried to access null heap memory.");

        return this->data_;
    }

    /*!
     * Compares @p size against the size() of the data buffer. If @p size is larger,
     * triggers an assertion failure. Otherwise, returns @p size.
     * 
     * This function's intended use case is to act as a drop-in bounds-check for
     * e.g. the mem* functions.
     * 
     * @code
     * heap_mem<char> buffer(1000);
     * 
     * unsigned x = 10;
     * unsigned y = 5;
     * 
     * // We want to memset x*y bytes of the data buffer. The size_check() function
     * // ensures that this value doesn't exceed the buffer's maximum byte capacity.
     * std::memset(buffer.data(), 0, buffer.size_check(x * y));
     * @endcode
     */
    unsigned size_check(const unsigned size) const
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
     * heap_mem<int> ints(2);
     * 
     * heap_mem<int> intsRef;
     * intsRef.point_to(ints);
     * 
     * ints[0] = 1234;
     * // intsRef[0] == 1234.
     * 
     * intsRef[0] = 1;
     * // ints[0] == 1.
     * 
     * ints.release();
     * // ints.is_null() == true.
     * // intsRef.is_null() == false.
     * 
     * // Undefined behavior, because the source buffer was released. The runtime
     * // bounds-checking of operator[] doesn't work, because it still thinks the
     * // data buffer exists.
     * intsRef[0];
     * 
     * intsRef.release();
     * // intsRef.is_null() == true.
     * 
     * // Will now correctly trigger the bounds-checking in operator[].
     * intsRef[0];
     * @endcode
     * 
     * @see
     * point_to(T *const, const int)
     */
    void point_to(heap_mem<T> &other)
    {
        k_assert(!other.is_null(), "Can't point to a null memory object.");

        return this->point_to(other.data(), other.count());
    }

    /*!
     * Sets the data buffer pointer to point to a region of memory pointed to by
     * @p dataPtr and which holds @p numElements elements of type @p T.
     * 
     * @code
     * int ints[10];
     * heap_mem<int> intsRef;
     * 
     * intsRef.point_to(ints, 10);
     * 
     * ints[0] = 1234;
     * // intsRef[0] == 1234.
     * @endcode
     * 
     * @see
     * point_to(heap_mem<T> &)
     */
    void point_to(T *const dataPtr, const int numElements)
    {
        k_assert(!this->data_, "Can't assign a pointer to a non-null memory object.");
        k_assert((numElements > 0), "Can't assign less than 1 element.");

        this->data_ = dataPtr;
        this->elementCount_ = numElements;
        this->alias = true;

        return;
    }

    /*!
     * Allocates memory for the data buffer.
     * 
     * @code
     * // Allocate memory for 10 ints using the constructor.
     * heap_mem<int> ints(10);
     * 
     * // Allocate memory for 10 ints using allocate().
     * heap_mem<int> ints2;
     * ints2.allocate(10);
     * 
     * // Allocate memory for 10 ints, then re-allocate for only 5 ints.
     * {
     *     heap_mem<int> ints3(10);
     *     // ints3.count() == 10.
     * 
     *     ints3.release();
     *     // ints3.count() == 0.
     * 
     *     ints3.allocate(5);
     *     // ints3.count() == 5.
     * }
     * @endcode
     * 
     * @warning
     * Call release() before re-allocating. Attempting to allocate over an
     * existing data buffer will trigger an assertion failure.
     */
    void allocate(const int elementCount, const char *const reason = nullptr)
    {
        k_assert(!this->data_, "Attempting to doubly allocate.");

        const unsigned numBytes = (sizeof(T) * elementCount);

        this->data_ = (T*)kmem_allocate(numBytes, (reason? reason : "<No reason given>"));
        this->elementCount_ = elementCount;

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
     * heap_mem<int> ints;
     * // ints.is_null() == true.
     * 
     * ints.allocate(1);
     * // ints.is_null() == false.
     * 
     * ints.release();
     * // ints.is_null() == true.
     * 
     * // This call is ignored, because the buffer has already been released.
     * ints.release();
     * // ints.is_null() == true.
     * @endcode
     * 
     * @code
     * heap_mem<int> ints(2);
     * 
     * heap_mem<int> intsRef;
     * intsRef.point_to(ints);
     * 
     * // Calling release() on a data buffer allocated with point_to()
     * // only sets that memory object's data pointer to null without releasing
     * // the buffer.
     * intsRef.release();
     * // intsRef.is_null() == true.
     * // ints.is_null() == false.
     * @endcode
     * 
     * @see
     * kmem_release()
     */
    void release(void)
    {
        if (this->data_ == nullptr)
        {
            DEBUG(("Was asked to release a null pointer in the memory cache. Ignoring this."));
            return;
        }

        if (!this->alias)
        {
            kmem_release((void**)&this->data_);
            this->elementCount_ = 0;
        }
        else
        {
            DEBUG(("Marking aliased memory to null instead of releasing it."));
            this->data_ = nullptr;
            this->elementCount_ = 0;
        }

        return;
    }

    /*!
     * Returns the number of elements (of type @p T) allocated for the data buffer.
     * 
     * @code
     * heap_mem<int> ints(11);
     * // ints.count() == 11.
     * // ints.size() == (11 * sizeof(int)).
     * @endcode
     * 
     * @code
     * heap_mem<int> ints;
     * // ints.count() == 0.
     * @endcode
     *
     * @see
     * size()
     */
    unsigned count(void) const
    {
        return this->elementCount_;
    }

    /*!
     * Returns the number of bytes allocated for the data buffer.
     *
     * @code
     * heap_mem<int> ints(11);
     * // ints.size() == (11 * sizeof(int)).
     * // ints.count() == 11.
     * @endcode
     *
     * @see
     * count()
     */
    unsigned size(void) const
    {
        return (sizeof(T) * this->count());
    }

    /*!
     * Returns true if the memory object currently has a null data buffer; false
     * otherwise.
     * 
     * @code
     * // The default constructor doesn't allocate the data buffer.
     * heap_mem<int> ints;
     * // ints.is_null() == true.
     * 
     * ints.allocate(1);
     * // ints.is_null() == false.
     * 
     * ints.release();
     * // ints.is_null() == true.
     * @endcode
     * 
     * @code
     * heap_mem<int> ints(2);
     * // ints.is_null() == false.
     * 
     * heap_mem<int> intsRef;
     * // intsRef.is_null() == true.
     * 
     * intsRef.point_to(ints);
     * // intsRef.is_null() == false.
     * @endcode
     * 
     * @see
     * allocate()
     */
    bool is_null(void) const
    {
        return !bool(this->data_);
    }

private:
    T *data_ = nullptr;

    // The number of elements (of type T) allocated for the data buffer.
    unsigned elementCount_ = 0;

    // Set to true if the data buffer was allocated with point_to(), i.e. if
    // this object doesn't own the data buffer.
    bool alias = false;
};

#endif
