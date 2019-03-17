/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 * 
 * For recording capture output into a video file.
 *
 * Uses OpenCV and x264 to produce a H.264 video.
 *
 */

#include <QFileInfo>
#include "../common/globals.h"
#include "../scaler/scaler.h"
#include "../display/display.h"
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

// Prepare the OpenCV video writer for recording frames into a video.
void krecord_initialize_recording(const char *const filename,
                                  const uint width, const uint height,
                                  const int frameRate)
{
    k_assert(!videoWriter.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    recordingParams.filename = filename;
    recordingParams.resolution = {width, height, 0};
    recordingParams.frameRate = frameRate;

    #if _WIN32
        // Encoder: x264vfw. Container: AVI.
        if (QFileInfo(filename).suffix() != "avi") recordingParams.filename += ".avi";
        videoWriter.open(filename, cv::VideoWriter::fourcc('X','2','6','4'), frameRate, cv::Size(width, height));
    #elif __linux__
        // Encoder: x264. Container: MP4.
        if (QFileInfo(filename).suffix() != "mp4") recordingParams.filename += ".mp4";
        videoWriter.open(filename, cv::VideoWriter::fourcc('a','v','c','1'), frameRate, cv::Size(width, height));
    #else
        #error "Unknown platform."
    #endif

    if (!videoWriter.isOpened())
    {
        kd_show_headless_error_message("VCS: Recording could not be started",
                                       "An error was encountred while attempting to start recording.\n\n"
                                       "More information may be found in the console window.");
    }

    return;
}

bool krecord_is_recording(void)
{
    return videoWriter.isOpened();
}

// Encode VCS's most recent output frame into the video.
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

    #if _WIN32
        videoWriter << cv::Mat(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
    #elif __linux__
        // Convert the frame into a format compatible with the video codec.
        cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
        cv::Mat convertedFrame(resolution.h, resolution.w, CV_8UC4, scratchBuffer.ptr());
        cv::cvtColor(originalFrame, convertedFrame, CV_BGRA2BGR);

        videoWriter << convertedFrame;
    #else
        #error "Unknown platform."
    #endif

    return;
}

void krecord_finalize_recording(void)
{
    videoWriter.release();

    return;
}
