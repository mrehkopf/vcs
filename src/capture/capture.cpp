/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS capture
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "common/propagate/propagate.h"
#include "capture/capture_api_virtual.h"
#include "capture/capture_api_rgbeasy.h"
#include "capture/capture_api_video4linux.h"
#include "capture/capture.h"

static capture_api_s *API = nullptr;

capture_api_s& kc_capture_api(void)
{
    k_assert(API, "Attempting to fetch the capture API prior to its initialization.");
    
    return *API;
}

void kc_initialize_capture(void)
{
    API =
    #ifdef CAPTURE_API_VIRTUAL
        new capture_api_virtual_s;
    #elif CAPTURE_API_RGBEASY
        new capture_api_rgbeasy_s;
    #elif CAPTURE_API_VIDEO4LINUX
        new capture_api_video4linux_s;
    #else
        #error "Unknown capture API."
    #endif

    API->initialize();

    kpropagate_news_of_new_capture_video_mode();

    return;
}

void kc_release_capture(void)
{
    DEBUG(("Releasing the capture API."));

    API->release();

    delete API;
    API = nullptr;

    return;
}
