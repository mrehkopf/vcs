#include <chrono>
#include "capture/capture_api_virtual.h"

bool capture_api_virtual_s::initialize(void)
{
    this->frameBuffer.r = this->defaultResolution;
    this->frameBuffer.pixels.alloc(MAX_FRAME_SIZE);

    return true;
}

bool capture_api_virtual_s::release(void)
{
    this->frameBuffer.pixels.release_memory();

    return true;
}

const captured_frame_s &capture_api_virtual_s::get_frame_buffer(void) const
{
    return this->frameBuffer;
}

capture_event_e capture_api_virtual_s::pop_capture_event_queue(void)
{
    // Normally, the capture card's output rate limits VCS's frame rate;
    // but for the virtual capture device, we'll need to emulate a delay.
    static auto startTime = std::chrono::system_clock::now();
    std::chrono::duration<double> timeDelta = (std::chrono::system_clock::now() - startTime);

    if (std::chrono::duration_cast<std::chrono::milliseconds>(timeDelta).count() <= 16)
    {
        return capture_event_e::none;
    }
    else
    {
        startTime = std::chrono::system_clock::now();

        return capture_event_e::new_frame;
    }
}

void capture_api_virtual_s::mark_frame_buffer_as_processed(void)
{
    this->animate_frame_buffer();

    return;
}

void capture_api_virtual_s::animate_frame_buffer(void)
{
    static unsigned offset = 0;
    offset++;

    for (unsigned y = 0; y < this->frameBuffer.r.h; y++)
    {
        for (unsigned x = 0; x < this->frameBuffer.r.w; x++)
        {
            const unsigned idx = ((x + y * this->frameBuffer.r.w) * (this->frameBuffer.r.bpp / 8));

            this->frameBuffer.pixels[idx + 0] = ((offset + x) % 256);
            this->frameBuffer.pixels[idx + 1] = ((offset + y) % 256);
            this->frameBuffer.pixels[idx + 2] = 150;
            this->frameBuffer.pixels[idx + 3] = 255;
        }
    }

    return;
}
