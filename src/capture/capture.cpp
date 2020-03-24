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
#include "capture/capture.h"

static capture_api_s *API = nullptr;

capture_api_s& kc_capture_api(void)
{
    return *API;
}

void kc_initialize_capture(void)
{
    API = new capture_api_rgbeasy_s;

    API->initialize();

    kpropagate_news_of_new_capture_video_mode();

    return;
}

void kc_release_capture(void)
{
    API->release();

    delete API;
    API = nullptr;

    return;
}
