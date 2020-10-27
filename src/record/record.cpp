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
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFuture>
#include "common/propagate/app_events.h"
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/memory/memory.h"
#include "record/record.h"

#ifdef USE_OPENCV
    #include <opencv2/core/core.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/videoio/videoio.hpp>

    static cv::VideoWriter VIDEO_WRITER;
#endif

// A scratch buffer used to convert from 32-bit color (input frames) to 24-bit
// color (for streaming into video).
static heap_bytes_s<u8> COLORCONV_BUFFER;

// Used to keep track of the recording's frame rate. Counts the number
// of frames captured between two points in time, and derives from that
// and the amount of time elapsed an estimate of the frame rate.
static struct framerate_estimator_s
{
    uint prevFrameCount = 0;
    double fps = 0;
    QElapsedTimer timer;

    void initialize(const uint frameCount)
    {
        this->fps = 0;
        this->prevFrameCount = frameCount;
        this->timer.start();

        return;
    }

    void update(const uint frameCount)
    {
        const uint frames = (frameCount - this->prevFrameCount);

        this->fps = (frames / (this->timer.nsecsElapsed() / 1000000000.0));
        this->prevFrameCount = frameCount;
        this->timer.restart();

        return;
    }

    double framerate(void)
    {
        return this->fps;
    }

} FRAMERATE_ESTIMATE;

static struct recording_s
{
    // We'll run the recording's video encoding in a separate thread.
    QFuture<void> encoderThread;

    // Metainfo.
    struct info_s
    {
        // The file into which the video is saved.
        std::string filename;

        // The video's resolution.
        resolution_s resolution;

        uint playbackFrameRate;

        // Number of frames recorded in this video.
        uint numFrames;

        // Number of frames we've had to drop (= not record), e.g. because the
        // encoder thread had not yet finished its work by the time a new frame
        // was available.
        uint numDroppedFrames;

        // Milliseconds passed since the recording was started.
        QElapsedTimer recordingTimer;
    } meta;
} RECORDING;

void krecord_initialize(void)
{
    COLORCONV_BUFFER.alloc(MAX_OUTPUT_WIDTH * MAX_OUTPUT_HEIGHT * 3);

    ke_events().scaler.newFrame->subscribe([]
    {
        if (krecord_is_recording())
        {
            krecord_record_current_frame();
        }
    });

    ke_events().recorder.recordingStarted->subscribe([]
    {
        DEBUG(("Recording into \"%s\".", RECORDING.meta.filename.c_str()));
    });

    ke_events().recorder.recordingEnded->subscribe([]
    {
        DEBUG(("Finished recording into \"%s\".", RECORDING.meta.filename.c_str()));
    });

    return;
}

// Prepare the OpenCV video writer for recording frames into a video.
// Returns true if successful, false otherwise.
//
bool krecord_start_recording(const char *const filename,
                             const uint width, const uint height,
                             const uint frameRate)
{
#ifndef USE_OPENCV
    kd_show_headless_info_message("VCS can't start recording",
                                  "OpenCV is needed for recording, but has been disabled on this build of VCS.");

    (void)filename;
    (void)width;
    (void)height;
    (void)frameRate;
    (void)linearFrameInsertion;

    return false;
#else
    k_assert(!VIDEO_WRITER.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    if ((width % 2 != 0) || (height % 2 != 0))
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "To record to video, the output resolution's width and height must "
                                       "be divisible by two (e.g. 640 x 480 but not 641 x 480).");
        return false;
    }

    if (!filename || !strlen(filename))
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "The output file is unnamed.");
        return false;
    }

    RECORDING.meta.filename = filename;
    RECORDING.meta.resolution = {width, height, 24};
    RECORDING.meta.playbackFrameRate = frameRate;
    RECORDING.meta.numFrames = 0;
    RECORDING.meta.numDroppedFrames = 0;
    RECORDING.meta.recordingTimer.start();
    FRAMERATE_ESTIMATE.initialize(0);

    #if _WIN32
        // Encoder: x264vfw. Container: AVI.
        if (QFileInfo(filename).suffix() != "avi") RECORDING.meta.filename += ".avi";
        const auto encoder = cv::VideoWriter::fourcc('X','2','6','4');
    #elif __linux__
        // Encoder: x264. Container: MP4.
        if (QFileInfo(filename).suffix() != "mp4") RECORDING.meta.filename += ".mp4";
        const auto encoder = cv::VideoWriter::fourcc('a','v','c','1');
    #else
        #error "Unknown platform."
    #endif

    VIDEO_WRITER.open(RECORDING.meta.filename,
                      encoder,
                      RECORDING.meta.playbackFrameRate,
                      cv::Size(RECORDING.meta.resolution.w, RECORDING.meta.resolution.h));

    if (!VIDEO_WRITER.isOpened())
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "An error was encountred while attempting to start recording. "
                                       "More information may be found in the console window.");
        return false;
    }

    ke_events().recorder.recordingStarted->fire();

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
    return RECORDING.meta.playbackFrameRate;
}

uint krecord_num_frames_dropped(void)
{
    return RECORDING.meta.numDroppedFrames;
}

double krecord_recording_framerate(void)
{
    return FRAMERATE_ESTIMATE.framerate();
}

std::string krecord_video_filename(void)
{
    return RECORDING.meta.filename;
}

uint krecord_num_frames_recorded(void)
{
    return RECORDING.meta.numFrames;
}

i64 krecord_recording_time(void)
{
    return RECORDING.meta.recordingTimer.elapsed();
}

resolution_s krecord_video_resolution(void)
{
    k_assert(krecord_is_recording(), "Querying video resolution while recording is inactive.");

    return RECORDING.meta.resolution;
}

static void encode_current_frame(void)
{
    VIDEO_WRITER << cv::Mat(RECORDING.meta.resolution.h, RECORDING.meta.resolution.w, CV_8UC3, COLORCONV_BUFFER.ptr());
    RECORDING.meta.numFrames++;

    return;
}

// Encode VCS's most recent output frame into the video.
//
void krecord_record_current_frame(void)
{
#ifdef USE_OPENCV
    k_assert(VIDEO_WRITER.isOpened(),
             "Attempted to record a video frame before video recording had been initialized.");

    // Get the current output frame.
    const resolution_s resolution = ks_output_resolution();
    const u8 *const frameData = ks_scaler_output_as_raw_ptr();
    if (frameData == nullptr) return;

    k_assert((resolution.w == RECORDING.meta.resolution.w &&
              resolution.h == RECORDING.meta.resolution.h), "Incompatible frame for recording: mismatched resolution.");

    // Save the frame into the video file.
    {
        cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
        cv::Mat frame = cv::Mat(resolution.h, resolution.w, CV_8UC3, COLORCONV_BUFFER.ptr());
        cv::cvtColor(originalFrame, frame, CV_BGRA2BGR);

        if (RECORDING.encoderThread.isFinished())
        {
            RECORDING.encoderThread = QtConcurrent::run(encode_current_frame);
        }
        else
        {
            RECORDING.meta.numDroppedFrames++;
        }

        /// TODO: Have a timer or something for this, rather than calling it
        /// every frame. Ideally, the timer setup would be handled by the GUI.
        kd_update_video_recording_metainfo();
    }

    return;
#endif
}

void krecord_stop_recording(void)
{
#ifdef USE_OPENCV
    RECORDING.encoderThread.waitForFinished();
    VIDEO_WRITER.release();

    ke_events().recorder.recordingEnded->fire();

    return;
#endif
}
