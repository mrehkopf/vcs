/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 * 
 * For recording capture output into a video file.
 *
 * Uses OpenCV's wrapper for x264 to produce a H.264 video. Expects the user to
 * have an x264 encoder available on their system.
 *
 */

#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <future>
#include <mutex>
#include <cstring>
#include "common/propagate/app_events.h"
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/memory/memory.h"
#include "record/record.h"
#include "record/recording_meta.h"
#include "record/recording_buffer.h"
#include "record/framerate_estimator.h"
#include "capture/capture.h"
#include "capture/capture_device.h"

#ifdef USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/videoio/videoio.hpp>

    static cv::VideoWriter VIDEO_WRITER;
#endif

static const unsigned RECORDING_BUFFER_CAPACITY = 10;

static recording_buffer_s RECORDING_BUFFER;

static framerate_estimator_s FRAMERATE_ESTIMATE;

static recording_meta_s RECORDING;

void krecord_initialize(void)
{
    RECORDING_BUFFER.initialize(RECORDING_BUFFER_CAPACITY);

    ke_events().scaler.newFrame.subscribe([]
    {
        if (krecord_is_recording())
        {
            krecord_record_current_frame();
        }
    });

    ke_events().recorder.recordingStarted.subscribe([]
    {
        DEBUG(("Recording into \"%s\".", RECORDING.filename.c_str()));
    });

    ke_events().recorder.recordingEnded.subscribe([]
    {
        DEBUG(("Finished recording into \"%s\".", RECORDING.filename.c_str()));
    });

    return;
}

static bool runEncoder = false;
static std::string encoderError = {};

#ifdef USE_OPENCV
// A thread launched by the recorder to encode and save to disk any captured
// frames.
static bool encoder_thread(void)
{
    while (runEncoder)
    {
        try
        {
            bool gotNewFrame = false;

            // See if the recorder thread has sent us any new frames. If so, copy
            // the oldest such frame's data into our scratch buffer, so we can
            // work on it and let the recorder thread reuse the memory.
            {
                std::lock_guard<std::mutex> lock(RECORDING_BUFFER.mutex);

                if (!RECORDING_BUFFER.empty())
                {
                    RECORDING.peakBufferUsagePercent = std::max(RECORDING_BUFFER.usage(),
                                                                RECORDING.peakBufferUsagePercent);

                    memcpy(RECORDING_BUFFER.scratchBuffer.ptr(),
                           RECORDING_BUFFER.pop()->ptr(),
                           RECORDING_BUFFER.scratchBuffer.up_to(RECORDING_BUFFER.maxWidth * RECORDING_BUFFER.maxHeight * 3));

                    gotNewFrame = true;
                }
            }

            if (gotNewFrame)
            {
                VIDEO_WRITER << cv::Mat(RECORDING.resolution.h,
                                        RECORDING.resolution.w,
                                        CV_8UC3,
                                        RECORDING_BUFFER.scratchBuffer.ptr());

                RECORDING.numFrames++;
            }
        }
        catch(const std::exception &e)
        {
            runEncoder = false;
            encoderError = e.what();

            return false;
        }
        catch(...)
        {
            runEncoder = false;
            encoderError = "Unknown error.";

            return false;
        }
    }

    return true;
}
#endif

// Prepare the OpenCV video writer for recording frames into a video.ssss
// Returns true if successful, false otherwise.
//
bool krecord_start_recording(const char *const filename,
                             const uint width,
                             const uint height,
                             const uint frameRate)
{
#ifndef USE_OPENCV
    kd_show_headless_info_message("VCS can't start recording",
                                  "Recording functionality is not available in this build of VCS. "
                                  "(Missing OpenCV.)");

    (void)filename;
    (void)width;
    (void)height;
    (void)frameRate;

    return false;
#else
    k_assert(!VIDEO_WRITER.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    if ((width > RECORDING_BUFFER.maxWidth) ||
        (height > RECORDING_BUFFER.maxHeight))
    {
        const std::string errString = "The maximum supported resolution is " +
                                      std::to_string(RECORDING_BUFFER.maxWidth) +
                                      " x " +
                                      std::to_string(RECORDING_BUFFER.maxHeight) +
                                      ".";

        kd_show_headless_error_message("VCS can't start recording", errString.c_str());
                                       
        return false;
    }

    if (((width % 2) != 0) ||
        ((height % 2) != 0))
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "The resolution's width and height must be divisible "
                                       "by two. For example, 640 x 480 but not 641 x 480.");

        return false;
    }

    if (!filename || !strlen(filename))
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "No output file specified.");

        return false;
    }

    RECORDING.filename = filename;
    RECORDING.resolution = {width, height, 24};
    RECORDING.playbackFrameRate = frameRate;
    RECORDING.numFrames = 0;
    RECORDING.numDroppedFrames = 0;
    RECORDING.peakBufferUsagePercent = 0;
    RECORDING_BUFFER.reset();
    FRAMERATE_ESTIMATE.initialize(0);

    #if _WIN32
        // Encoder: x264vfw. Container: AVI.
        if (QFileInfo(filename).suffix() != "avi") RECORDING.filename += ".avi";
        const auto encoder4cc = cv::VideoWriter::fourcc('X','2','6','4');
    #elif __linux__
        // Encoder: x264. Container: MP4.
        if (QFileInfo(filename).suffix() != "mp4") RECORDING.filename += ".mp4";
        const auto encoder4cc = cv::VideoWriter::fourcc('a','v','c','1');
    #else
        #error "Unknown platform."
    #endif

    VIDEO_WRITER.open(RECORDING.filename,
                      encoder4cc,
                      RECORDING.playbackFrameRate,
                      cv::Size(RECORDING.resolution.w, RECORDING.resolution.h));

    RECORDING.numGlobalDroppedFrames = kc_capture_device().get_missed_frames_count();
    RECORDING.recordingTimer.start();
    runEncoder = true;
    encoderError = "No error specified.";
    RECORDING.encoderThreadFuture = std::async(std::launch::async, encoder_thread);

    if (!VIDEO_WRITER.isOpened())
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "An error was encountred while attempting to start recording. "
                                       "More information may be found in the console window.");
        return false;
    }

    ke_events().recorder.recordingStarted.fire();

    return true;
#endif
}

bool krecord_is_recording(void)
{
#ifdef USE_OPENCV
    return VIDEO_WRITER.isOpened();
#else
    return false;
#endif
}

uint krecord_playback_framerate(void)
{
    return RECORDING.playbackFrameRate;
}

uint krecord_num_frames_dropped(void)
{
    return RECORDING.numDroppedFrames;
}

double krecord_recording_framerate(void)
{
    return FRAMERATE_ESTIMATE.framerate();
}

std::string krecord_video_filename(void)
{
    return RECORDING.filename;
}

uint krecord_num_frames_recorded(void)
{
    return RECORDING.numFrames;
}

uint krecord_peak_buffer_usage_percent(void)
{
    return RECORDING.peakBufferUsagePercent;
}

i64 krecord_recording_time(void)
{
    return RECORDING.recordingTimer.elapsed();
}

resolution_s krecord_video_resolution(void)
{
    k_assert(krecord_is_recording(), "Querying video resolution while recording is inactive.");

    return RECORDING.resolution;
}

// Encode VCS's most recent output frame into the video.
//
void krecord_record_current_frame(void)
{
#ifdef USE_OPENCV
    k_assert(VIDEO_WRITER.isOpened(),
             "Attempted to record a video frame before video recording had been initialized.");

    // If the encoder thread has, for some reason, had to exit early. This will
    // probably have been due to an error of some kind.
    if (!runEncoder)
    {
        const std::string errorMsg = "The VCS video recorder has encountered an error and must "
                                     "stop recording.\n\nThe following error message was reported: "
                                     "\"" + encoderError + "\"\n\nFurther information may have "
                                     "been printed into the console.";

        kd_show_headless_error_message("Recording interrupted", errorMsg.c_str());
                                       
        krecord_stop_recording();

        return;
    }

    // Get the current output frame.
    const resolution_s resolution = ks_scaler_output_resolution();
    const u8 *const frameData = ks_scaler_output_pixels_ptr();
    if (frameData == nullptr) return;

    k_assert((resolution.w == RECORDING.resolution.w &&
              resolution.h == RECORDING.resolution.h), "Incompatible frame for recording: mismatched resolution.");

    // Queue the frame to be encoded by the encoder thread.
    {
        // Copy the frame's data into the recording buffers. Note that we convert
        // the frame from 32-bit color to 24-bit color.
        {
            u8 *bufferPtr = nullptr;

            // Fetch a new memory slot from the frame buffer.
            {
                std::lock_guard<std::mutex> lock(RECORDING_BUFFER.mutex);

                if (RECORDING_BUFFER.full())
                {
                    RECORDING.numDroppedFrames++;
                }
                else
                {
                    bufferPtr = RECORDING_BUFFER.push()->ptr();
                }
            }

            if (bufferPtr)
            {
                /// TODO: Verify that the frames' resolution doesn't exceed the
                /// frame buffer's maximum resolution. We alredy have a check in
                /// place for this before we allow the recording to start, but you
                /// never know...
                
                cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
                cv::Mat frame = cv::Mat(resolution.h, resolution.w, CV_8UC3, bufferPtr);
                cv::cvtColor(originalFrame, frame, CV_BGRA2BGR);
            }
        }

        RECORDING.numDroppedFrames += (kc_capture_device().get_missed_frames_count() - RECORDING.numGlobalDroppedFrames);
        RECORDING.numGlobalDroppedFrames = kc_capture_device().get_missed_frames_count();
    }

    return;
#endif
}

void krecord_release(void)
{
    RECORDING_BUFFER.release();

    return;
}

void krecord_stop_recording(void)
{
#ifdef USE_OPENCV
    if (RECORDING.encoderThreadFuture.valid())
    {
        runEncoder = false;
        RECORDING.encoderThreadFuture.wait();
    }

    VIDEO_WRITER.release();

    ke_events().recorder.recordingEnded.fire();

    return;
#endif
}
