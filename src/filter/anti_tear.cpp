/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear engine
 *
 * Applies anti-tearing to input frames by detecting which portions of the frame
 * changed since the previous one and accumulating changed portions in a backbuffer
 * until the entire frame has been constructed.
 *
 */

#include <cstring>
#include "filter/anti_tear.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "common/memory/memory.h"
#include "common/disk/csv.h"

/*
 * TODOS:
 *
 * - experimental code, needs cleaning up.
 *
 * - extend the coverage of the memory manager. at the moment, the individual
 *   frame buffers operate on raw pointers all of the time. without a mem checker,
 *   could be causing issues.
 *
 */

static anti_tear_options_s DEFAULT_SETTINGS;

// Parameters for tear detection.
static u32 MAXY = 0, MAXY_OFFS = 0;
static u32 MINY = 0;
static u32 DOMAIN_SIZE = 8;
static u32 STEP_SIZE = 1;
static u32 MATCHES_REQD = 11;
static u32 THRESHOLD = 3;

static bool VISUALIZE = false;
static bool VISUALIZE_TEAR = true;
static bool VISUALIZE_RANGE = true;

// The color depth we expect frames to be when they're fed into the anti-tear engine.
static const u32 EXPECTED_BIT_DEPTH = 32;

static bool ANTI_TEARING_ENABLED = false;

// We'll place the extracted portions of frames into two back buffers. If a frame
// contains new data both for the previous frame and the next frame, we place the
// data for the previous frame into one back buffer and the data for the next frame
// into the other back buffer, i.e. effectively getting triple buffering.
static const u32 NUM_BACK_BUFFERS = 2;
static heap_bytes_s<u8> BACK_BUFFER_STORAGE;

// Pointers to the two back buffers such that A will be the next frame to display,
// and B will contain data for the next set of frames.
static tear_frame_s BUFFER_PRIMARY = {0};
static tear_frame_s BUFFER_SECONDARY = {0};

// The tears in the current frame.
static frame_tears_s CURRENT_TEARS = {0};

// Set to true if we don't want any calls to at_reset_buffers() to do anything.
static bool PREVENT_BUFFER_RESET = false;

// If there are more tears than this in the frame, we can't process it.
static const u32 MAX_NUM_TEARS_PER_FRAME = 2;

// The pixel data of the previous frame we received.
static heap_bytes_s<u8> PREV_FRAME;
static resolution_s PREV_FRAME_RES = {0};

// A vertical strip that describes for each row whether it's data matches that of
// the previous frame (0) or is new (1).
static int TEAR_STRIP[MAX_OUTPUT_HEIGHT];

static void reset_buffer(tear_frame_s *const b)
{
    b->newDataStart = MAXY;
    b->isDone = false;

    return;
}

static void reset_all_buffers(void)
{
    if (PREVENT_BUFFER_RESET)
    {
        return;
    }

    reset_buffer(&BUFFER_PRIMARY);
    reset_buffer(&BUFFER_SECONDARY);

    memset(TEAR_STRIP, 0, sizeof(int) * MAX_OUTPUT_HEIGHT);
    memset(&CURRENT_TEARS, 0, sizeof(frame_tears_s));

    return;
}

void kat_set_buffer_updates_disabled(const bool disabled)
{
    if (PREVENT_BUFFER_RESET && !disabled)
    {
        PREVENT_BUFFER_RESET = false;
        reset_all_buffers();
    }
    else
    {
        PREVENT_BUFFER_RESET = true;
    }

    return;
}

static void mark_tear_strip_as_invalid(void)
{
    TEAR_STRIP[0] = -1;

    return;
}

static bool tear_strip_is_valid(void)
{
    return bool(TEAR_STRIP[0] != -1);
}

// See whether the tear strip appears valid, i.e. contains no suspicious values.
//
static void validate_tear_strip(void)
{
    CURRENT_TEARS.numTears = 0;
    int curBlockType = TEAR_STRIP[0];

    for (uint i = 1; i < MAXY; i++)
    {
        // Count the number of tears we find. I.e. if the current value isn't
        // the same as the previous value, this is a tear.
        if (TEAR_STRIP[i] != curBlockType)
        {
            CURRENT_TEARS.numTears++;
            if (CURRENT_TEARS.numTears > MAX_NUM_TEARS_PER_FRAME)
            {
                mark_tear_strip_as_invalid();

                goto done;
            }

            CURRENT_TEARS.tearY[CURRENT_TEARS.numTears - 1] = i;
            CURRENT_TEARS.newData[CURRENT_TEARS.numTears - 1] = curBlockType;

            curBlockType = TEAR_STRIP[i];
        }
    }

    if (CURRENT_TEARS.numTears == 0)
    {
        mark_tear_strip_as_invalid();

        goto done;
    }

    // If the bottom isn't new but the primary buffer's bottom is MAXY, this is invalid.
    if (CURRENT_TEARS.newData[CURRENT_TEARS.numTears - 1] &&
        CURRENT_TEARS.tearY[CURRENT_TEARS.numTears - 1] < (MAXY - 1) &&
        BUFFER_PRIMARY.newDataStart == MAXY)
    {
        mark_tear_strip_as_invalid();

        goto done;
    }

    if (BUFFER_PRIMARY.newDataStart == MAXY &&
        BUFFER_SECONDARY.newDataStart == MAXY)
    {
        goto done;
    }

    // At least one of the tears must align with a previous termination point.
    for (uint i = 0; i < CURRENT_TEARS.numTears; i++)
    {
        if (CURRENT_TEARS.tearY[i] == BUFFER_PRIMARY.newDataStart)
        {
            goto done;
        }

        if (CURRENT_TEARS.tearY[i] == BUFFER_SECONDARY.newDataStart )
        {
            goto done;
        }
    }

    mark_tear_strip_as_invalid();

    done:
    return;
}

#if 0 // Not used at the moment; but since the anti-tear code is still work-in-progress, let's leave this up for now.
    static void cleanup_tear_strip(void)
    {
        int numTears = 0;
        bool curBlockType = TEAR_STRIP[0];

        for (uint y = 1; y < MAXY; y++)
        {
            // Clean up isolated islands.
            /*if (TEAR_STRIP[i] != curBlockType &&
                (TEAR_STRIP[i - 1] == curBlockType &&
                 TEAR_STRIP[i + 1] == curBlockType))
            {
                TEAR_STRIP[i] = curBlockType;
            }*/
        }

        done:
        return;
    }
#endif

static void update_tear_strip(const captured_frame_s &frame)
{
    memset(TEAR_STRIP, 0, sizeof(int) * MAXY);
    memset(&CURRENT_TEARS, 0, sizeof(frame_tears_s));

    // Loop over the vertical range set by the user.
    for (size_t y = MINY; y < MAXY; y++)
    {
        u32 x = 0;
        u32 matches = 0;
        const int lim = THRESHOLD * DOMAIN_SIZE;

        // Slide a sampling window across this horizontal row of pixels.
        while ((x + DOMAIN_SIZE) < frame.r.w)
        {
            int oldR = 0, oldG = 0, oldB = 0;
            int newR = 0, newG = 0, newB = 0;

            // Find the average color values of the current and the previous frame
            // within this sampling window.
            for (size_t w = 0; w < DOMAIN_SIZE; w++)
            {
                const int idx = ((x + w) + y * frame.r.w) * 4;

                oldB += PREV_FRAME[idx + 0];
                oldG += PREV_FRAME[idx + 1];
                oldR += PREV_FRAME[idx + 2];

                newB += frame.pixels[idx + 0];
                newG += frame.pixels[idx + 1];
                newR += frame.pixels[idx + 2];
            }

            // If the averages differ by enough. Essentially by having used an
            // average of multiple pixels (across the sampling window) instead
            // of comparing individual pixels, we're reducing the effect of
            // random capture noise that's otherwise hard to remove.
            if (abs(oldR - newR) > lim ||
                abs(oldG - newG) > lim ||
                abs(oldB - newB) > lim)
            {
                matches++;
            }

            // If we've found that the averages have differed substantially
            // enough times, we conclude that this row of pixels is different from
            // the previous frame, i.e. that it's new data.
            if (matches >= MATCHES_REQD)
            {
                TEAR_STRIP[y] = 1;

                break;
            }

            x += STEP_SIZE;
        }
    }

    //at_cleanup_tear_strip();

    validate_tear_strip();

    return;
}

static void save_as_previous_frame(const captured_frame_s &frame)
{
    memcpy(PREV_FRAME.ptr(), frame.pixels.ptr(), frame.r.w * frame.r.h * (frame.r.bpp / 8));
    PREV_FRAME_RES = frame.r;

    return;
}

// Visualize the various settings for the anti-tear engine.
//
static void visualize_settings(captured_frame_s &frame)
{
    if (!VISUALIZE || !VISUALIZE_RANGE)
    {
        return;
    }

    // Shade the area under the scan range.
    for (unsigned long y = MINY; y < MAXY; y++)
    {
        for (unsigned long x = 0; x < frame.r.w; x++)
        {
            const unsigned long idx = ((x + y * frame.r.w) * 4);

            frame.pixels[idx + 1] *= 0.5;
            frame.pixels[idx + 2] *= 0.5;
        }
    }

    // Indicate with a line where the sampling area starts and ends.
    u8 color = 255;
    for (unsigned long x = 0; x < frame.r.w; x++)
    {
        int idx = (x + MINY * frame.r.w) * 4;
        frame.pixels[idx + 0] = color;
        frame.pixels[idx + 1] = !color? 255 : 0;
        frame.pixels[idx + 2] = !color? 255 : 0;

        idx = (x + (MAXY - 1) * frame.r.w) * 4;
        frame.pixels[idx + 0] = !color? 255 : 0;
        frame.pixels[idx + 1] = color;
        frame.pixels[idx + 2] = !color? 255 : 0;

        if (x % 30 == 0)
        {
            color = ~color;
        }
    }

    return;
}

// Draws the uptear of the current frame, to visualize where the frame was
// cut.
//
static void visualize_tearing(captured_frame_s &frame)
{
    if (!VISUALIZE || !VISUALIZE_TEAR)
    {
        return;
    }

    for (uint i = 0; i < CURRENT_TEARS.numTears; i++)
    {
        // Indicate this tear with a horizontal line.
        u8 color = 210;
        for (unsigned long x = 0; x < frame.r.w; x++)
        {
            const int idx = (x + CURRENT_TEARS.tearY[i] * frame.r.w) * 4;

            frame.pixels[idx + 0] = color;
            frame.pixels[idx + 1] = color;
            frame.pixels[idx + 2] = color;

            if (x % 5 == 0)
            {
                color = (color == 210)? 30 : 210;
            }
        }
    }

    return;
}

static bool copy_new_frame_data(const captured_frame_s &frame)
{
    u32 y = 0;

    #define COPY_DATA_INTO(buffer, term) buffer.newDataStart = y;\
                                         for (; y < term; y++)\
                                         {\
                                             const int idx = (y * frame.r.w) * (frame.r.bpp / 8);\
                                             memcpy(buffer.pixels + idx, frame.pixels.ptr() + idx, frame.r.w * (frame.r.bpp / 8));\
                                         }\
                                         if (buffer.newDataStart == 0)\
                                         {\
                                             buffer.isDone = true;\
                                         }\

    for (uint i = 0; i < CURRENT_TEARS.numTears; i++)
    {
        const bool isBottomBlock = bool(i == (CURRENT_TEARS.numTears - 1));

        // Continue copying the new data into the primary or secondary buffer.
        if (CURRENT_TEARS.tearY[i] == BUFFER_PRIMARY.newDataStart)
        {
            COPY_DATA_INTO(BUFFER_PRIMARY, CURRENT_TEARS.tearY[i]);
        }
        else if (CURRENT_TEARS.tearY[i] == BUFFER_SECONDARY.newDataStart)
        {
            COPY_DATA_INTO(BUFFER_SECONDARY, CURRENT_TEARS.tearY[i]);
        }
        // Otherwise, we have a new block of data at the bottom that doesn't
        // continue either buffer's earlier copies but instead starts a new frame.
        else if (isBottomBlock)
        {
            y = CURRENT_TEARS.tearY[i];

            if (BUFFER_PRIMARY.newDataStart == MAXY)
            {
                COPY_DATA_INTO(BUFFER_PRIMARY, frame.r.h);
            }
            else if (BUFFER_SECONDARY.newDataStart == MAXY)
            {
                COPY_DATA_INTO(BUFFER_SECONDARY, frame.r.h);
            }
            else
            {
                //printf("\t\tReceived data for a new frame, but both back buffers were in use.\n");
                return false;
            }
        }
    }

    return true;
}

void kat_set_anti_tear_enabled(const bool state)
{
    ANTI_TEARING_ENABLED = state;

    reset_all_buffers();

    return;
}

bool kat_is_anti_tear_enabled(void)
{
    return ANTI_TEARING_ENABLED;
}

void kat_set_visualization(const bool visualize,
                           const bool visualizeTear,
                           const bool visualizeRange)
{
    VISUALIZE = visualize;
    VISUALIZE_TEAR = visualizeTear;
    VISUALIZE_RANGE = visualizeRange;

    return;
}

void kat_set_range(const u32 min, const u32 max)
{
    MINY = min;
    MAXY_OFFS = max;

    reset_all_buffers();

    return;
}

void kat_set_threshold(const u32 t)
{
    THRESHOLD = t;

    reset_all_buffers();

    return;
}

void kat_set_domain_size(const u32 ds)
{
    DOMAIN_SIZE = ds;

    reset_all_buffers();

    return;
}

void kat_set_step_size(const u32 s)
{
    STEP_SIZE = s;

    reset_all_buffers();

    return;
}

void kat_set_matches_required(const u32 mr)
{
    MATCHES_REQD = mr;

    reset_all_buffers();

    return;
}

u8* kat_anti_tear(u8 *const pixels, const resolution_s &r)
{
    captured_frame_s frame;
    frame.r = r;
    frame.pixels.point_to(pixels, (frame.r.w * frame.r.h * (frame.r.bpp / 8)));

    k_assert(pixels != nullptr,
             "The anti-tear engine expected a pixel buffer, but received null.");

    k_assert(r.bpp == EXPECTED_BIT_DEPTH,
             "The anti-tear engine expected a certain bit depth, but the input frame did not comply.");

    if (!ANTI_TEARING_ENABLED)
    {
        return pixels;
    }

    // Update the range over which we'll operate.
    MAXY = (int(frame.r.h - MAXY_OFFS) < 0)? 0 : (frame.r.h - MAXY_OFFS);
    if (MAXY <= MINY)
    {
        MAXY = (MINY + 1);
    }
    if (MAXY > frame.r.h)
    {
        MAXY = frame.r.h;
    }
    if (MAXY < 1)
    {
        MAXY = 1;
    }
    if (MINY >= MAXY)
    {
        MINY = (MAXY - 1);
    }
    if (MINY >= frame.r.h)
    {
        MINY = (frame.r.h - 1);
    }

    // We can only properly anti-tear if the resolution doesn't change in the
    // meantine. If it changed, just bail out.
    if (PREV_FRAME_RES.w != r.w ||
        PREV_FRAME_RES.h != r.h)
    {
        save_as_previous_frame(frame);

        goto fail;
    }

    // Find which areas of the frame have changed since last time.
    update_tear_strip(frame);

    // Save this frame for comparing the next frame against.
    save_as_previous_frame(frame);

    // If the tear strip isn't valid, i.e. we can't reliably process it for tears,
    // just return whatever frame we were passed and discard our internal buffers'
    // contents.
    if (!tear_strip_is_valid())
    {
        //captured_frame_s f = {pixels, frame.r};

        visualize_settings(frame);

       // printf("invalid frame for anti-tear\n");

        goto fail;
    }

    // We expect that the primary buffer will be completed before the secondary
    // one. If that hasn't been the case here, something's gone wrong.
    if (BUFFER_SECONDARY.isDone &&
        !BUFFER_PRIMARY.isDone)
    {
        goto fail;
    }

    // Otherwise, copy any new data from the frame into the buffers.
    if (!copy_new_frame_data(frame))
    {
        goto fail;
    }

    // If we've finished with the primary back buffer, let it be drawn.
    // Also, flip the buffers then so that the secondary back buffer
    // becomes the primary one.
    if (BUFFER_PRIMARY.isDone)
    {
        u8 *const p = BUFFER_PRIMARY.pixels;
        captured_frame_s f;
        f.r = frame.r;
        f.pixels.point_to(p, (f.r.w * f.r.h * (f.r.bpp / 8)));

        visualize_tearing(f);
        visualize_settings(f);

        reset_buffer(&BUFFER_PRIMARY);

        #define SWITCH_BUFFERS(a, b) tear_frame_s tmp = (a);\
                                     (a) = (b);\
                                     (b) = tmp;

        SWITCH_BUFFERS(BUFFER_PRIMARY, BUFFER_SECONDARY);

        #undef SWITCH_BUFFERS

        return p;
    }

    // No frame was ready for display, so signal to keep displaying the frame that
    // was already being displayed.
    return nullptr;

    fail:
    reset_all_buffers();
    return pixels;
}

void kat_initialize_anti_tear(void)
{
    const resolution_s &maxres = kc_capture_api().get_maximum_resolution();

    INFO(("Initializing the anti-tear engine for %u x %u max.", maxres.w, maxres.h));

    const u32 backBufferSize = maxres.w * maxres.h * (EXPECTED_BIT_DEPTH / 8);
    BACK_BUFFER_STORAGE.alloc(backBufferSize * 2, "Anti-tearing backbuffers");
    BUFFER_PRIMARY.pixels = (BACK_BUFFER_STORAGE.ptr() + 0);
    BUFFER_SECONDARY.pixels = (BACK_BUFFER_STORAGE.ptr() + backBufferSize);

    PREV_FRAME.alloc((maxres.w * maxres.h * (EXPECTED_BIT_DEPTH / 8)), "Anti-tearing prev frame buffer");

    reset_all_buffers();

    DEFAULT_SETTINGS.matchesReqd = 11;
    DEFAULT_SETTINGS.stepSize = STEP_SIZE;
    DEFAULT_SETTINGS.rangeDown = MINY;
    DEFAULT_SETTINGS.rangeUp = MAXY_OFFS;
    DEFAULT_SETTINGS.threshold = THRESHOLD;
    DEFAULT_SETTINGS.windowLen = DOMAIN_SIZE;

    return;
}

const anti_tear_options_s& kat_default_settings(void)
{
    return DEFAULT_SETTINGS;
}

void kat_release_anti_tear(void)
{
    DEBUG(("Releasing the anti-tear engine."));

    BACK_BUFFER_STORAGE.release_memory();
    PREV_FRAME.release_memory();

    return;
}
