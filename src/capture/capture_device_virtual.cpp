/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifdef CAPTURE_DEVICE_VIRTUAL

#include <chrono>
#include "common/propagate/app_events.h"
#include "capture/capture_device_virtual.h"
#include "common/timer/timer.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;
static double CURRENT_REFRESH_RATE = 0;

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static bool IS_VALID_SIGNAL = true;

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

bool capture_device_virtual_s::initialize(void)
{
    this->frameBuffer.r = {640, 480, 32};
    this->frameBuffer.pixelFormat = capture_pixel_format_e::rgb_888;
    this->frameBuffer.pixels.alloc(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Capture frame buffer (virtual)");

    // Simulate the capturing of a new frame.
    kt_timer(std::round(1000 / TARGET_REFRESH_RATE), [this](const unsigned)
    {
        if ((this->get_resolution().bpp > MAX_CAPTURE_BPP) ||
            (this->get_resolution().w > MAX_CAPTURE_WIDTH) ||
            (this->get_resolution().h > MAX_CAPTURE_HEIGHT))
        {
            IS_VALID_SIGNAL = false;
            push_capture_event(capture_event_e::invalid_signal);
        }
        else
        {
            IS_VALID_SIGNAL = true;
            this->refresh_test_pattern();
            push_capture_event(capture_event_e::new_frame);
        }
    });

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        const double newRefreshRate = (NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs));

        NUM_FRAMES_PER_SECOND = 0;

        if (CURRENT_REFRESH_RATE != newRefreshRate)
        {
            CURRENT_REFRESH_RATE = newRefreshRate;
            push_capture_event(capture_event_e::new_video_mode);
        }
    });

    return true;
}

bool capture_device_virtual_s::release(void)
{
    this->frameBuffer.pixels.release_memory();

    return true;
}

void capture_device_virtual_s::refresh_test_pattern(void)
{
    static unsigned numFramesGenerated = 0;

    numFramesGenerated++;
    NUM_FRAMES_PER_SECOND++;

    for (unsigned y = 0; y < this->frameBuffer.r.h; y++)
    {
        for (unsigned x = 0; x < this->frameBuffer.r.w; x++)
        {
            const unsigned idx = ((x + y * this->frameBuffer.r.w) * (this->frameBuffer.r.bpp / 8));
            const u8 red = ((numFramesGenerated + x) % 256);
            const u8 green = ((numFramesGenerated + y) % 256);
            const u8 blue = 150;
            const u8 alpha = 255;

            switch (this->frameBuffer.pixelFormat)
            {
                case capture_pixel_format_e::rgb_888:
                {
                    this->frameBuffer.pixels[idx + 0] = red;
                    this->frameBuffer.pixels[idx + 1] = green;
                    this->frameBuffer.pixels[idx + 2] = blue;
                    this->frameBuffer.pixels[idx + 3] = alpha;

                    break;
                }
                case capture_pixel_format_e::rgb_565:
                {
                    const u16 pixel = (((blue  / 8) << 11) |
                                       ((green / 4) << 5)  |
                                       ((red   / 8) << 0));

                    *((u16*)&this->frameBuffer.pixels[idx]) = pixel;

                    break;
                }
                case capture_pixel_format_e::rgb_555:
                {
                    const u16 pixel = (((alpha / 256) << 11) |
                                       ((blue  / 8)   << 10) |
                                       ((green / 8)   << 5)  |
                                       ((red   / 8)   << 0));

                    *((u16*)&this->frameBuffer.pixels[idx]) = pixel;

                    break;
                }
                default: break;
            }
        }
    }

    return;
}

bool capture_device_virtual_s::set_pixel_format(const capture_pixel_format_e pf)
{
    switch (pf)
    {
        case capture_pixel_format_e::rgb_888:
        {
            this->frameBuffer.r.bpp = 32;
            break;
        }
        case capture_pixel_format_e::rgb_565:
        case capture_pixel_format_e::rgb_555:
        {
            this->frameBuffer.r.bpp = 16;
            break;
        }
    }

    this->frameBuffer.pixelFormat = pf;

    push_capture_event(capture_event_e::new_video_mode);

    return true;
}

bool capture_device_virtual_s::set_resolution(const resolution_s &r)
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

refresh_rate_s capture_device_virtual_s::get_refresh_rate() const
{
    return refresh_rate_s(CURRENT_REFRESH_RATE);
}

bool capture_device_virtual_s::set_input_channel(const unsigned idx)
{
    this->inputChannelIdx = idx;

    ke_events().capture.newInputChannel.fire();

    return true;
}

bool capture_device_virtual_s::has_invalid_signal(void) const
{
    return !IS_VALID_SIGNAL;
}

const captured_frame_s& capture_device_virtual_s::get_frame_buffer(void) const
{
    return this->frameBuffer;
}

capture_event_e capture_device_virtual_s::pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
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
