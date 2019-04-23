/*
 * 2018, 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * An integration test to validate scaler functionality. Kind of a
 * kludge until a more refined testing system is in place.
 * 
 * Will print out "Successfully validated" or "Failed to validate",
 * depending on whether the test succeeded, and exit with either
 * EXIT_SUCCESS or EXIT_FAILURE likewise.
 *
 */

#include "scaling.h"
#include "capture/capture.h"
#include "scaler/scaler.h"
#include "filter/filter.h"
#include "common/globals.h"

static const char ASPECT_TO_TEST[] = "Scaling";

#ifndef USE_OPENCV
    #error "OpenCV must be enabled for scaler validation. See toggle in the .pro file."
#endif

int ktest_integration_scaling(void)
{
    captured_frame_s f;

    try
    {
        k_assert(((kc_hardware_max_capture_resolution().w <= MAX_OUTPUT_WIDTH) &&
                    (kc_hardware_max_capture_resolution().h <= MAX_OUTPUT_HEIGHT)),
                    "The maximum input size should not be larger than the maximum output size.");

        k_assert((ks_max_output_bit_depth() >= 32), "Support for 32-bit output depth is required for this test.");

        // Initialize the system subset to be tested.
        ks_initialize_scaler();
        kc_initialize_capturer();
        kf_initialize_filters();
        kf_clear_filters();

        // Initialize the frame to be tested with.
        f.pixels.alloc(MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT * (ks_max_output_bit_depth() / 8), "Test frame buffer");

        // Test the scaler.
        {
            // Ask to use invalid scalers. The system should ignore these requests.
            INFO(("SCALING: testing invalid scaler assignment..."));
            QString upName = ks_upscaling_filter_name();
            QString downName = ks_downscaling_filter_name();
            ks_set_downscaling_filter(NULL);
            ks_set_upscaling_filter("_^-_______________________");
            k_assert((ks_upscaling_filter_name() == upName) &&
                        (ks_downscaling_filter_name() == downName),
                        "Detected possibly invalid scaler assignment.");

            // Test invalid frame scaling. The scaler should reject processing these.
            INFO(("SCALING: testing invalid resolutions..."));
            for (uint y: {kc_hardware_min_capture_resolution().h-5, kc_hardware_max_capture_resolution().h+5})
            {
                for (uint x: {kc_hardware_min_capture_resolution().w-5, kc_hardware_max_capture_resolution().w+5})
                {
                    for (uint bpp: {0, 16, 24, 956})
                    {
                        f.processed = false;
                        f.r = {x, y, bpp};

                        kc_VALIDATION_set_capture_color_depth(bpp);
                        ks_scale_frame(f);

                        k_assert(!f.processed, "An invalid frame may have been accepted for display.");
                    }
                }
            }

            // Test invalid frame scaling. The scaler should accept these.
            INFO(("SCALING: testing valid resolutions..."));
            for (uint y: {kc_hardware_min_capture_resolution().h + ((kc_hardware_max_capture_resolution().h - kc_hardware_min_capture_resolution().h) / 2),
                            kc_hardware_min_capture_resolution().h + ((kc_hardware_max_capture_resolution().h - kc_hardware_min_capture_resolution().h) / 4)})
            {
                for (uint x: {kc_hardware_min_capture_resolution().w + ((kc_hardware_max_capture_resolution().w - kc_hardware_min_capture_resolution().w) / 2),
                                kc_hardware_min_capture_resolution().w + ((kc_hardware_max_capture_resolution().w - kc_hardware_min_capture_resolution().w) / 4)})
                {
                    for (uint bpp: {16, 32})
                    {
                        f.processed = false;
                        f.r = {x, y, bpp};

                        kc_VALIDATION_set_capture_pixel_format(bpp == 32? RGB_PIXELFORMAT_888 : RGB_PIXELFORMAT_565);
                        kc_VALIDATION_set_capture_color_depth(bpp);
                        ks_scale_frame(f);

                        k_assert(f.processed, "A valid frame was not accepted for display");
                    }
                }
            }
        }

        // Test the scaler's pixel output, by writing some known color values into
        // the frame's pixel buffer, passing the frame through the scaler, and then
        // sampling the output (screen) buffer's contents.
        {
            INFO(("SCALING: testing scaler output..."));
            k_assert(ks_downscaling_filter_name() == "Nearest", "Expected the nearest downscaler for this test.");
            const u8 r = 45, g = 117, b = 150, a = 255;
            const u16 p16_565 =         (r/8)   | ((g/4)   << 5)   | ((b/8)   << 11);
            const u32 p16_565_as_p32 =  (r/8*8) | ((g/4*4) << 8)   | ((b/8*8) << 16);
            const u16 p16_555 =         (r/8)   | ((g/8)   << 5)   | ((b/8)   << 10);
            const u32 p16_555_as_p32 =  (r/8*8) | ((g/8*8) << 8)   | ((b/8*8) << 16);
            const u32 p32 =              r      |  (g      << 8)   |  (b      << 16)   | (a << 24);

            INFO(("\t32-bit (888)..."));
            const resolution_s outres = ks_output_resolution();
            const resolution_s outresHalf = {outres.w / 2, outres.h / 2, outres.bpp};

            f.r = {outresHalf.w, outresHalf.h, 32};
            for (uint i = 0; i < (f.r.w * f.r.h); i++)
            {
                ((u32*)f.pixels.ptr())[i] = p32;
            }
            kc_VALIDATION_set_capture_color_depth(32);
            kc_VALIDATION_set_capture_pixel_format(RGB_PIXELFORMAT_888);
            ks_clear_scaler_output_buffer();
            ks_scale_frame(f);
            for (uint i = 0; i < (outres.w * outres.h); i++)
            {
                k_assert(((u32*)ks_VALIDATION_raw_output_buffer_ptr())[i] == p32, "Possible corruption in a scaled image.");
            }

            INFO(("\t16-bit (565)..."));
            f.r = {outresHalf.w, outresHalf.h, 16};
            for (uint i = 0; i < (f.r.w * f.r.h); i++)
            {
                ((u16*)f.pixels.ptr())[i] = p16_565;
            }
            kc_VALIDATION_set_capture_color_depth(16);
            kc_VALIDATION_set_capture_pixel_format(RGB_PIXELFORMAT_565);
            ks_clear_scaler_output_buffer();
            ks_scale_frame(f);
            for (uint i = 0; i < (outres.w * outres.h); i++)
            {
                k_assert((((u32*)ks_VALIDATION_raw_output_buffer_ptr())[i] & 0xffffff) == p16_565_as_p32, "Possible corruption in a scaled image.");
            }

            INFO(("\t16-bit (555)..."));
            f.r = {outresHalf.w, outresHalf.h, 16};
            for (uint i = 0; i < (f.r.w * f.r.h); i++)
            {
                ((u16*)f.pixels.ptr())[i] = p16_555;
            }
            kc_VALIDATION_set_capture_color_depth(16);
            kc_VALIDATION_set_capture_pixel_format(RGB_PIXELFORMAT_555);
            ks_clear_scaler_output_buffer();
            ks_scale_frame(f);
            for (uint i = 0; i < (outres.w * outres.h); i++)
            {
                k_assert((((u32*)ks_VALIDATION_raw_output_buffer_ptr())[i] & 0xffffff) == p16_555_as_p32, "Possible corruption in a scaled image.");
            }
        }

        // Release the subsystem.
        PROGRAM_EXIT_REQUESTED = 1;
        f.pixels.release_memory();
        ks_release_scaler();
        kc_release_capturer();
        kf_release_filters();
        kmem_deallocate_memory_cache();
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "Failed to validate '%s'. Encountered the following error: '%s'.\n", ASPECT_TO_TEST, e.what());
        return EXIT_FAILURE;
    }

    printf("Successfully validated: '%s'.\n", ASPECT_TO_TEST);
    INFO(("Bye."));
    return EXIT_SUCCESS;
}

int main(void)
{
    return ktest_integration_scaling();
}
