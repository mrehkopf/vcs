/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef CAPTURE_BACK_BUFFER_H
#define CAPTURE_BACK_BUFFER_H

#include <vector>
#include "common/memory/memory_interface.h"

struct capture_back_buffer_s
{
    capture_back_buffer_s(void);

    ~capture_back_buffer_s(void);

    heap_bytes_s<u8>& page(const unsigned idx);

    void allocate(void);

    void release(void);

    const unsigned numPages = 2;

private:
    std::vector<heap_bytes_s<u8>> pages;
};

#endif
