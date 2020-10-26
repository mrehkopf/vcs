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

// Incoming frames will first be accumulated into a frame buffer; and when the buffer
// is full, encoded into the video file.
// NOTE: The frame buffer expcts frames to be of 24-bit color depth (e.g. BGR).
struct frame_buffer_s
{
    u8* memoryPool = nullptr;

    resolution_s frameResolution;

    // How many frames the frame buffer is currently storing.
    uint numFrames = 0;

    // A timestamp of roughly when the corresponding frame was captured.
    std::vector<i64> frameTimestamps;

    // How many frames in total the buffer has memory capacity for.
    uint maxNumFrames = 0;

    ~frame_buffer_s(void)
    {
        delete[] memoryPool;

        return;
    }

    // Allocates the frame buffer for the given number of frames of the given
    // resolution. The frames' pixels are expected to have three 8-bit color
    // channels, each - e.g. RGB.
    void initialize(const uint width, const uint height, const uint frameCapacity)
    {
        delete[] memoryPool;
        memoryPool = new u8[width * height * 3 * frameCapacity];

        this->maxNumFrames = frameCapacity;
        this->numFrames = 0;
        this->frameResolution = {width, height, 0};
        this->frameTimestamps.resize(frameCapacity);

        return;
    }

    void reset(void)
    {
        this->numFrames = 0;

        return;
    }

    resolution_s resolution(void) const
    {
        return this->frameResolution;
    }

    bool is_full(void) const
    {
        return (this->numFrames >= this->maxNumFrames);
    }

    uint frame_count(void) const
    {
        return this->numFrames;
    }

    const std::vector<i64>& frame_timestamps(void) const
    {
        return frameTimestamps;
    }

    // Returns a pointer to an unused area of the frame buffer that has the
    // capacity to hold the pixels of a single frame.
    u8* next_slot(const i64 timestamp)
    {
        k_assert((this->numFrames < this->maxNumFrames), "Overflowing the video recording frame buffer.");

        const uint offset = ((this->frameResolution.w * this->frameResolution.h * 3) * this->numFrames);

        this->frameTimestamps.at(this->numFrames) = timestamp;
        this->numFrames++;

        return (memoryPool + offset);
    }

    // Index into the frame buffer on a per-frame basis. Note that this index
    // should be to an already-stored frame. If you want to access uninitialized
    // memory in the frame buffer, use the next_slot() function.
    u8* frame(const uint frameIdx) const
    {
        k_assert((frameIdx < this->maxNumFrames), "Attempting to access a frame buffer out of bounds.");
        k_assert((frameIdx < this->numFrames), "Attempting to access an uninitialized frame.");

        const uint offset = ((this->frameResolution.w * this->frameResolution.h * 3) * frameIdx);
        return (memoryPool + offset);
    }
};

static struct recording_s
{
    // Accumulate the captured frames in two back buffers. When one buffer
    // fills up, we'll flip the buffers and encode the filled-up one's frames
    // into the video file.
    frame_buffer_s *activeFrameBuffer;
    frame_buffer_s backBuffers[2];
    void flip_frame_buffer(void)
    {
        activeFrameBuffer = (activeFrameBuffer == &backBuffers[0])? &backBuffers[1] : &backBuffers[0];
    }

    // We'll run the recording's video encoding in a separate thread.
    QFuture<void> encoderThread;

    // If true, frames will be inserted into the video in linear time, not as
    // they come in. For instance, if the input FPS is 55 and the video's playback
    // rate is set to 60, linear insertion tries to ensure that frames are duplicated
    // and/or dropped to ensure roughly 60 FPS output into the video. Without linear
    // insertion, the frames would be output at 55 FPS, and playing the video at its
    // rate of 60 FPS would result in temporal skew.
    bool linearFrameInsertion = true;

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

        // Milliseconds passed since the recording was started.
        QElapsedTimer recordingTimer;
    } meta;
} RECORDING;

void krecord_initialize(void)
{
    ke_events().scaler.newFrame->subscribe([]
    {
        if (krecord_is_recording())
        {
            krecord_record_new_frame();
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
                             const uint frameRate,
                             const bool linearFrameInsertion,
                             const uint bufferCapacity)
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
    RECORDING.linearFrameInsertion = linearFrameInsertion;
    RECORDING.meta.numFrames = 0;
    RECORDING.meta.recordingTimer.start();
    FRAMERATE_ESTIMATE.initialize(0);

    // Allocate memory.
    try
    {
        RECORDING.backBuffers[0].initialize(width, height, bufferCapacity);
        RECORDING.backBuffers[1].initialize(width, height, bufferCapacity);
    }
    catch(...)
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "Failed to allocate memory for video frame buffers. "
                                       "The video's resolution may be too high.");
        return false;
    }
    RECORDING.activeFrameBuffer = &RECORDING.backBuffers[0];

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

void encode_frame_buffer(frame_buffer_s *const frameBuffer)
{
#ifdef USE_OPENCV
    if (RECORDING.linearFrameInsertion)
    {
        const auto &frameTimestamps = frameBuffer->frame_timestamps();

        // Nanoseconds between each frame at the recording's playback rate.
        const i64 stampDelta = ((1000.0 / RECORDING.meta.playbackFrameRate) * 1000000);

        // Add frames at even intervals as per the recording's playback rate.
        i64 stamp = (RECORDING.meta.numFrames * stampDelta);
        uint i = 0;
        while (stamp <= frameTimestamps[frameBuffer->frame_count()-1])
        {
            for (; i < frameBuffer->frame_count(); i++)
            {
                if (frameTimestamps[i] >= stamp)
                {
                    VIDEO_WRITER << cv::Mat(frameBuffer->resolution().h, frameBuffer->resolution().w, CV_8UC3, frameBuffer->frame(i));
                    RECORDING.meta.numFrames++;
                    break;
                }
            }

            stamp += stampDelta;
        }
    }
    else
    {
        for (uint i = 0; i < frameBuffer->frame_count(); i++)
        {
            VIDEO_WRITER << cv::Mat(frameBuffer->resolution().h, frameBuffer->resolution().w, CV_8UC3, frameBuffer->frame(i));
            RECORDING.meta.numFrames++;
        }
    }

    frameBuffer->reset();

    return;
#else
    (void)frameBuffer;
#endif
}

// Encode VCS's most recent output frame into the video.
//
void krecord_record_new_frame(void)
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

    // Convert the frame to BRG, and save it into the frame buffer.
    cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
    cv::Mat frame = cv::Mat(resolution.h, resolution.w, CV_8UC3, RECORDING.activeFrameBuffer->next_slot(RECORDING.meta.recordingTimer.nsecsElapsed()));
    cv::cvtColor(originalFrame, frame, CV_BGRA2BGR);

    // Once we've accumulated enough frames to fill the frame buffer, encode
    // its contents into the video file.
    if (RECORDING.activeFrameBuffer->is_full())
    {
        RECORDING.encoderThread.waitForFinished();

        FRAMERATE_ESTIMATE.update(RECORDING.meta.numFrames);
        kd_update_video_recording_metainfo();

        // Run the encoding in a separate thread.
        const auto frameBuffer = RECORDING.activeFrameBuffer;
        RECORDING.encoderThread = QtConcurrent::run([=]{encode_frame_buffer(frameBuffer);});

        // Meanwhile, switch to the other frame buffer, and keep accumulating new frames.
        RECORDING.flip_frame_buffer();
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
