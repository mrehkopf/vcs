/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include "capture/back_buffer.h"

capture_back_buffer_s::capture_back_buffer_s()
{
    this->pages.resize(this->numPages);
}

capture_back_buffer_s::~capture_back_buffer_s()
{
    this->release();
}

heap_bytes_s<u8>& capture_back_buffer_s::page(const unsigned idx)
{
    k_assert((idx < this->numPages), "Accessing back buffer pages out of bounds.");

    return this->pages[idx];
}

void capture_back_buffer_s::allocate()
{
    for (auto &page: this->pages)
    {
        page.alloc(MAX_FRAME_SIZE);
    }
}

void capture_back_buffer_s::release()
{
    for (auto &page: this->pages)
    {
        if (!page.is_null())
        {
            page.release_memory();
        }
    }
}
