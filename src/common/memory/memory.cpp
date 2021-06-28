/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS memory manager
 *
 * A basic memory manager. Allocates a bunch of memory on initialization, then doles
 * it out to callers in small sequential chunks afterwards. Useful to prevent fragmentation
 * and to see how memory is being used by the program, though a limited implementation.
 *
 */

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "common/memory/memory_interface.h"
#include "common/memory/memory.h"
#include "common/globals.h"

/*
 * TODOS:
 *
 * - add dynamic resizing to the memory buffer for when it's running short.
 *
 */

// How many bytes to pre-allocate for the memory buffer. Note that the buffer
// currently has no ability to grow itself, so this amount must be sufficient
// for the whole of the program's execution.
static uint MEMORY_BUFFER_SIZE = 256u/*MB*/ * 1024 * 1024;

// The memory manager isn't prepared for multithreading, so discourage its use in
// that context. We'll grab the id of whatever thread initializes this, and later
// bail if any other thread requests operations of the manager.
static const std::thread::id NATIVE_THREAD_ID = std::this_thread::get_id();

// A single allocation from the memory buffer.
struct mem_allocation_s
{
    char reason[64];    // For what purpose the memory was needed.
    const void *memory;
    uint32_t numBytes;
    bool isInUse;       // Set to false if the owner asks the memory manager to release the memory.
    bool isReused;      // Whether this allocation used to belong to someone else who since had it released.
};

static int TOTAL_BYTES_ALLOCATED = 0;
static int TOTAL_BYTES_RELEASED = 0;

static u8 *MEMORY_BUFFER = NULL;

// A pointer to the next free byte in the memory buffer (allocations from the
// buffer are made sequentially).
static u8 *NEXT_FREE = NULL;

// Each allocation requested is logged into the allocation table.
static const uint NUM_ALLOC_TABLE_ELEMENTS = 512;
static mem_allocation_s *ALLOC_TABLE = NULL;

static void release(void)
{
    if (MEMORY_BUFFER == NULL)
    {
        DEBUG(("The memory subsystem wasn't initialized; skipping releasing it."));
        return;
    }

    k_assert(PROGRAM_EXIT_REQUESTED, "Was asked to release the memory subsystem before the program had been told to exit.");

    const unsigned numMBAllocated = (TOTAL_BYTES_ALLOCATED / 1024);
    const unsigned percentUtilization = int((double(TOTAL_BYTES_ALLOCATED) / MEMORY_BUFFER_SIZE) * 100);

    DEBUG(("atexit: Releasing the memory subsystem at %d%% utilization (%u KB).",
           percentUtilization,
           numMBAllocated));

    #ifndef RELEASE_BUILD
        for (uint i = 0; i < NUM_ALLOC_TABLE_ELEMENTS; i++)
        {
            if (ALLOC_TABLE[i].memory &&
                ALLOC_TABLE[i].isInUse)
            {
                DEBUG(("ORPHANED: %u bytes at %p (\"%s\")",
                       ALLOC_TABLE[i].numBytes,
                       ALLOC_TABLE[i].memory,
                       ALLOC_TABLE[i].reason));
            }
        }
    #endif

    free(MEMORY_BUFFER);

    return;
}

static void initialize(void)
{
    k_assert((MEMORY_BUFFER == NULL), "Attempting to re-initialize the memory subsystem.");
    static_assert(std::is_same<decltype(MEMORY_BUFFER), u8*>::value && (sizeof(u8) == 1), "Expected the memory buffer elements to be u8*.");

    INFO(("Initializing the memory subsystem with a buffer of %u MB.", (MEMORY_BUFFER_SIZE / 1024 / 1024)));

    MEMORY_BUFFER = (u8*)calloc(MEMORY_BUFFER_SIZE, 1);
    NEXT_FREE = MEMORY_BUFFER;
    k_assert(MEMORY_BUFFER != NULL, "The memory manager failed to allocate enough memory for its operation.");

    // Use the beginning of the memory buffer for the allocation table.
    k_assert((ALLOC_TABLE == NULL), "Expected a null allocation table.");
    const uint allocTableByteSize = (NUM_ALLOC_TABLE_ELEMENTS * sizeof(mem_allocation_s));
    k_assert((allocTableByteSize < MEMORY_BUFFER_SIZE), "Not enough room in the memory buffer for the allocation table.");
    ALLOC_TABLE = (mem_allocation_s*)MEMORY_BUFFER;
    TOTAL_BYTES_ALLOCATED += allocTableByteSize;
    NEXT_FREE += allocTableByteSize;

    std::atexit(release);

    return;
}

void* kmem_allocate(const int numBytes, const char *const reason)
{
    k_assert(std::this_thread::get_id() == NATIVE_THREAD_ID, "Potential attempt to allocate memory from multiple threads. This isn't supported.");
    k_assert(!PROGRAM_EXIT_REQUESTED, "No more memory should be allocated after the program has been asked to terminate.");
    k_assert(numBytes > 0, "Can't allocate sub-byte memory blocks.");

    // Initialize the memory buffer the first time the allocator is called.
    if (MEMORY_BUFFER == NULL)
    {
        initialize();
    }

    k_assert(NEXT_FREE != NULL, "Didn't have a valid free slot in the memory buffer for a new allocation.");
    u8 *mem = NEXT_FREE;

    // Find a free spot in the allocation table.
    uint tableIdx = 0;
    bool useOldEntry = false;   // If we decided to use a previously-cleared entry.
    while (1)
    {
        // Take the first unallocated entry. These will always be at the end of
        // the allocation chain, since past allocations are never cleared until
        // the program exits.
        if (ALLOC_TABLE[tableIdx].memory == NULL)
        {
            break;
        }

        // But also, if there's a released entry of just the right size, we can
        // take that one instead.
        if (!ALLOC_TABLE[tableIdx].isInUse &&
            ALLOC_TABLE[tableIdx].numBytes == uint(numBytes))
        {
            useOldEntry = true;
            break;
        }

        tableIdx++;
        k_assert(tableIdx < NUM_ALLOC_TABLE_ELEMENTS,
                 "Memory manager overflowed while looking for room for an allocation.");
    }

    if (useOldEntry)
    {
        mem = (u8*)ALLOC_TABLE[tableIdx].memory;
        ALLOC_TABLE[tableIdx].isReused = true;
    }
    else
    {
        TOTAL_BYTES_ALLOCATED += numBytes;
        NEXT_FREE += numBytes;
    }

    k_assert(strlen(reason) < NUM_ELEMENTS(ALLOC_TABLE[tableIdx].reason),
             "The reason given for an allocation was too long to fit into the string array.");
    strcpy(ALLOC_TABLE[tableIdx].reason, reason);
    ALLOC_TABLE[tableIdx].memory = mem;
    ALLOC_TABLE[tableIdx].numBytes = numBytes;
    ALLOC_TABLE[tableIdx].isInUse = true;

    // Validate the memory before sending it off.
    k_assert((mem + numBytes) < (MEMORY_BUFFER + MEMORY_BUFFER_SIZE),
             "Memory allocation would overflow the memory buffer. The buffer needs to be made larger.");
    k_assert(mem != NULL, "Detected a null return from the memory manager allocator. This should not be the case.");
    memset(mem, 0, numBytes);

    return (void*)mem;
}

static uint alloc_table_index_of_pointer(const void *const mem)
{
    uint tableIdx = 0;
    while (ALLOC_TABLE[tableIdx].memory != mem)
    {
        tableIdx++;
        if (tableIdx >= NUM_ALLOC_TABLE_ELEMENTS)
        {
            k_assert(0, "Couldn't find the requested allocation in the allocation table.");
            goto bail;
        }
    }

    return tableIdx;

    bail:
    throw std::runtime_error("Unexpected code path in the memory manager.");    // We should never reach this, but will if asserts are disabled.
    return 0;
}

uint kmem_sizeof_allocation(const void *const mem)
{
    if (mem == NULL)
    {
        k_assert(0, "The memory manager was asked for the size of a null allocation.");
        return 0;
    }

    const uint idx = alloc_table_index_of_pointer(mem);

    return ALLOC_TABLE[idx].numBytes;
}

void kmem_release(void **mem)
{
    k_assert(std::this_thread::get_id() == NATIVE_THREAD_ID, "Potential attempt to release memory from multiple threads. This isn't supported.");

    if (*mem == NULL)
    {
        return;
    }

    // Make sure we've been asked to release a valid allocation.
    assert(*mem >= MEMORY_BUFFER);
    assert(*mem <= (MEMORY_BUFFER + MEMORY_BUFFER_SIZE));

    const uint idx = alloc_table_index_of_pointer(*mem);

    // Warn of double deletes.
    if (!ALLOC_TABLE[idx].isInUse)
    {
        NBENE(("Asked to double-delete memory at %p (%s).", *mem, ALLOC_TABLE[idx].reason));
        k_assert(0, "Double-deleting memory.");
    }

    ALLOC_TABLE[idx].isInUse = false;

    if (!ALLOC_TABLE[idx].isReused)
    {
        TOTAL_BYTES_RELEASED += ALLOC_TABLE[idx].numBytes;
    }

    *mem = NULL;

    return;
}
