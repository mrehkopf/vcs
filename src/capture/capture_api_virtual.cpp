/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIRTUAL

#include <chrono>
#include "common/propagate/app_events.h"
#include "capture/capture_api_virtual.h"
#include "common/timer/timer.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;
static double CURRENT_REFRESH_RATE = 0;
static auto REFRESH_RATE_TIMER = std::chrono::steady_clock::now();

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static void push_capture_event(const capture_event_e event)
{
    CAPTURE_EVENT_FLAGS[(int)event] = true;

    return;
}

static bool pop_capture_event(const capture_event_e event)
{
    const bool oldFlagValue = CAPTURE_EVENT_FLAGS[(int)event];

    CAPTURE_EVENT_FLAGS[(int)event] = false;

    return oldFlagValue;
}

bool capture_api_virtual_s::initialize(void)
{
    this->frameBuffer.r = {640, 480, 32};
    this->frameBuffer.pixelFormat = this->defaultPixelFormat;
    this->frameBuffer.pixels.alloc(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Capture frame buffer (virtual)");

    // Animate the screen's test pattern.
    kt_timer(std::round(1000 / TARGET_REFRESH_RATE), [this]
    {
        static unsigned numFramesGenerated = 0;

        numFramesGenerated++;
        NUM_FRAMES_PER_SECOND++;

        for (unsigned y = 0; y < this->frameBuffer.r.h; y++)
        {
            for (unsigned x = 0; x < this->frameBuffer.r.w; x++)
            {
                const unsigned idx = ((x + y * this->frameBuffer.r.w) * (this->frameBuffer.r.bpp / 8));

                this->frameBuffer.pixels[idx + 0] = ((numFramesGenerated + x) % 256);
                this->frameBuffer.pixels[idx + 1] = ((numFramesGenerated + y) % 256);
                this->frameBuffer.pixels[idx + 2] = 150;
                this->frameBuffer.pixels[idx + 3] = 255;
            }
        }

        push_capture_event(capture_event_e::new_frame);
    });

    kt_timer(1000, []
    {
        const auto timeNow = std::chrono::steady_clock::now();
        const auto realMsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - REFRESH_RATE_TIMER).count();
        const double newRefreshRate = (NUM_FRAMES_PER_SECOND * (1000.0 / realMsElapsed));

        NUM_FRAMES_PER_SECOND = 0;
        REFRESH_RATE_TIMER = std::chrono::steady_clock::now();

        if (CURRENT_REFRESH_RATE != newRefreshRate)
        {
            CURRENT_REFRESH_RATE = newRefreshRate;
            push_capture_event(capture_event_e::new_video_mode);
        }
    });

    return true;
}

bool capture_api_virtual_s::release(void)
{
    this->frameBuffer.pixels.release_memory();

    return true;
}

bool capture_api_virtual_s::set_resolution(const resolution_s &r)
{
    if ((r.w > this->maxResolution.w) ||
        (r.w < this->minResolution.w) ||
        (r.h > this->maxResolution.h) ||
        (r.h < this->minResolution.h))
    {
        return false;
    }

    this->frameBuffer.r.w = r.w;
    this->frameBuffer.r.h = r.h;

    push_capture_event(capture_event_e::new_video_mode);

    return true;
}

refresh_rate_s capture_api_virtual_s::get_refresh_rate() const
{
    return refresh_rate_s(CURRENT_REFRESH_RATE);
}

bool capture_api_virtual_s::set_input_channel(const unsigned idx)
{
    this->inputChannelIdx = idx;

    ke_events().capture.newInputChannel.fire();

    return true;
}

const captured_frame_s& capture_api_virtual_s::get_frame_buffer(void) const
{
    return this->frameBuffer;
}

capture_event_e capture_api_virtual_s::pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::new_video_mode))
    {
        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    return capture_event_e::none;
}

#endif
