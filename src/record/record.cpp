/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 * 
 * For recording capture output into a video file.
 *
 * Uses OpenCV, FFMPEG, and OpenH264 to produce a MP4/H.264 video.
 *
 */

#include <QFileInfo>
#include "../common/globals.h"
#include "../scaler/scaler.h"
#include "../common/memory.h"
#include "record.h"

#ifdef USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/videoio/videoio.hpp>
#endif

static cv::VideoWriter videoWriter;

// The parameters of the current recording.
struct params_s
{
    std::string filename;
    resolution_s resolution;
    int frameRate;
} recordingParams;

void krecord_initialize_recording(const char *const filename,
                                  const uint width, const uint height,
                                  const int frameRate)
{
    k_assert(!videoWriter.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    recordingParams.filename = filename;
    recordingParams.resolution = {width, height, 0};
    recordingParams.frameRate = frameRate;

    // Only the MP4 container is supported, so force it if need be.
    if (QFileInfo(filename).suffix() != "mp4") recordingParams.filename += ".mp4";

    videoWriter.open(filename, 0x21, frameRate, cv::Size(width, height));

    k_assert(videoWriter.isOpened(), "Failed to initialize the recording.");

    return;
}

bool krecord_is_recording(void)
{
    return videoWriter.isOpened();
}

void krecord_record_new_frame(void)
{
    static heap_bytes_s<u8> scratchBuffer(MAX_FRAME_SIZE, "Video recording conversion buffer.");

    k_assert(videoWriter.isOpened(),
             "Attempted to record a video frame before video recording had been initialized.");

    // Get the current output frame.
    const resolution_s resolution = ks_output_resolution();
    const u8 *const frameData = ks_scaler_output_as_raw_ptr();
    if (frameData == nullptr) return;

    k_assert((resolution.w == recordingParams.resolution.w &&
              resolution.h == recordingParams.resolution.h), "Incompatible frame for recording: mismatched resolution.");

    // Convert the frame into a format compatible with the video codec.
    cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
    cv::Mat convertedFrame(resolution.h, resolution.w, CV_8UC4, scratchBuffer.ptr());
    cv::cvtColor(originalFrame, convertedFrame, CV_BGRA2BGR);

    videoWriter << convertedFrame;

    return;
}

void krecord_finalize_recording(void)
{
    videoWriter.release();

    return;
}
