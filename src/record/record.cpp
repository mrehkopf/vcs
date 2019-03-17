/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 * 
 * For recording capture output into a video file.
 *
 */

#include "../common/globals.h"
#include "../scaler/scaler.h"
#include "../common/memory.h"
#include "record.h"

#ifdef USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/imgproc/imgproc.hpp>

    // For VideoWriter.
    #if CV_MAJOR_VERSION > 2
        #include <opencv2/videoio/videoio.hpp>
    #else
        #include <opencv2/highgui/highgui.hpp>
    #endif
#endif

static cv::VideoWriter videoWriter;

void krecord_initialize_recording(void)
{
    k_assert(!videoWriter.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    // OpenCV documentation: "FFMPEG backend with MP4 container natively uses other values as fourcc code".
    const auto h264Encoder = 0x21;

    videoWriter.open("test.mp4", h264Encoder, 60, cv::Size(640, 480));

    k_assert(videoWriter.isOpened(), "Failed to initialize the recording.");

    return;
}

bool krecord_is_recording(void)
{
    return videoWriter.isOpened();
}

void krecord_record_new_frame(void)
{
    static heap_bytes_s<u8> tmp(MAX_FRAME_SIZE, "Video recording conversion buffer.");

    k_assert(videoWriter.isOpened(),
             "Attempted to record a video frame before video recording had been initialized.");

    // Get the current output frame.
    const resolution_s r = ks_output_resolution();
    const u8 *const frameData = ks_scaler_output_as_raw_ptr();
    if (frameData == nullptr) return;

    // Convert the frame into a format compatible with the video codec.
    cv::Mat originalFrame(r.h, r.w, CV_8UC4, (u8*)frameData);
    cv::Mat convertedFrame(r.h, r.w, CV_8UC4, tmp.ptr());
    cv::cvtColor(originalFrame, convertedFrame, CV_BGRA2BGR);

    videoWriter << convertedFrame;

    return;
}

void krecord_finalize_recording(void)
{
    videoWriter.release();

    return;
}
