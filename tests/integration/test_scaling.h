/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef CAPTURE_TEST_H
#define CAPTURE_TEST_H

#ifdef VALIDATION_RUN
    #include "../types.h"
    #ifndef USE_RGBEASY_API
        #include "../null_rgbeasy.h"
    #endif

    // Helper functions to directly manipulate certain program-side capture
	// parameters, without involving the capture hardware.
    void kc_VALIDATION_set_capture_color_depth(const uint bpp);
    void kc_VALIDATION_set_capture_pixel_format(const PIXELFORMAT pf);
    const u8* ks_VALIDATION_raw_output_buffer_ptr(void);
#endif

#endif
