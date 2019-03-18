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

#include <QElapsedTimer>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QFuture>
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

static cv::VideoWriter VIDEO_WRITER;

// Incoming frames will first be accumulated into a frame buffer; and when the buffer
// is full, encoded into the video file.
// NOTE: The frame buffer expcts frames to be of 24-bit color depth (e.g. BGR).
struct frame_buffer_s
{
    heap_bytes_s<u8> memoryPool;

    resolution_s frameResolution;

    // How many frames the frame buffer is currently storing.
    uint numFrames = 0;

    // How many frames in total the buffer has memory capacity for.
    uint maxNumFrames = 0;

    ~frame_buffer_s()
    {
        if (!this->memoryPool.is_null()) this->memoryPool.release_memory();

        return;
    }

    // Allocates the frame buffer for the given number of frames of the given
    // resolution. The frames' pixels are expected to have three 8-bit color
    // channels, each - e.g. RGB.
    void initialize(const uint width, const uint height, const uint frameCapacity)
    {
        if (!this->memoryPool.is_null()) this->memoryPool.release_memory();
        this->memoryPool.alloc((width * height * 3 * frameCapacity), "Video recording frame buffer.");

        this->maxNumFrames = frameCapacity;
        this->numFrames = 0;
        this->frameResolution = {width, height, 0};

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

    // Returns a pointer to an unused area of the frame buffer that has the
    // capacity to hold the pixels of a single frame.
    u8* next_slot(void)
    {
        k_assert((this->numFrames < this->maxNumFrames), "Overflowing the video recording frame buffer.");

        const uint offset = ((this->frameResolution.w * this->frameResolution.h * 3) * this->numFrames);
        this->numFrames++;

        return (memoryPool.ptr() + offset);
    }

    // Index into the frame buffer on a per-frame basis. Note that this index
    // should be to an already-stored frame. If you want to access uninitialized
    // memory in the frame buffer, use the next_slot() function.
    u8* frame(const uint frameIdx) const
    {
        k_assert((frameIdx < this->maxNumFrames), "Attempting to access a frame buffer out of bounds.");
        k_assert((frameIdx < this->numFrames), "Attempting to access an uninitialized frame.");

        const uint offset = ((this->frameResolution.w * this->frameResolution.h * 3) * frameIdx);
        return (memoryPool.ptr() + offset);
    }
};

// The parameters of the current recording.
struct params_s
{
    // The file into which the video is saved.
    std::string filename;

    // The video's resolution.
    resolution_s resolution;

    // The video's playback framerate.
    uint frameRate;

    // Number of frames recorded in this video. Excludes frames that were
    // duplicated by the recorder to prevent time compression.
    uint numFrames;

    // Milliseconds passed since the recording was initialized.
    QElapsedTimer recordingTimer;

    frame_buffer_s *activeFrameBuffer;
    frame_buffer_s backBuffers[2];
    void flip_frame_buffer(void)
    {
        activeFrameBuffer = (activeFrameBuffer == &backBuffers[0])? &backBuffers[1] : &backBuffers[0];
    }

    QFuture<void> encoderThread;
} RECORDING;

// Prepare the OpenCV video writer for recording frames into a video.
// Returns true if successful, false otherwise.
//
bool krecord_start_recording(const char *const filename,
                             const uint width, const uint height,
                             const uint frameRate)
{
    k_assert(!VIDEO_WRITER.isOpened(),
             "Attempting to intialize a recording that has already been initialized.");

    RECORDING.filename = filename;
    RECORDING.resolution = {width, height, 0};
    RECORDING.frameRate = frameRate;
    RECORDING.numFrames = 0;
    RECORDING.recordingTimer.start();
    RECORDING.backBuffers[0].initialize(width, height, frameRate);
    RECORDING.backBuffers[1].initialize(width, height, frameRate);
    RECORDING.activeFrameBuffer = &RECORDING.backBuffers[0];

    #if _WIN32
        // Encoder: x264vfw. Container: AVI.
        if (QFileInfo(filename).suffix() != "avi") recordingParams.filename += ".avi";
        videoWriter.open(filename, cv::VideoWriter::fourcc('X','2','6','4'), frameRate, cv::Size(width, height));
    #elif __linux__
        // Encoder: x264. Container: MP4.
        if (QFileInfo(filename).suffix() != "mp4") RECORDING.filename += ".mp4";
        VIDEO_WRITER.open(filename, cv::VideoWriter::fourcc('a','v','c','1'), frameRate, cv::Size(width, height));
    #else
        #error "Unknown platform."
    #endif

    if (!VIDEO_WRITER.isOpened())
    {
        kd_show_headless_error_message("VCS: Recording could not be started",
                                       "An error was encountred while attempting to start recording.\n\n"
                                       "More information may be found in the console window.");
        return false;
    }

    return true;
}

bool krecord_is_recording(void)
{
    return VIDEO_WRITER.isOpened();
}

void encode_frame_buffer(frame_buffer_s *const frameBuffer)
{
    for (uint i = 0; i < frameBuffer->frame_count(); i++)
    {
        VIDEO_WRITER << cv::Mat(frameBuffer->resolution().h, frameBuffer->resolution().w, CV_8UC3, frameBuffer->frame(i));
    }

    frameBuffer->reset();

    return;
}

// Encode VCS's most recent output frame into the video.
//
void krecord_record_new_frame(void)
{
    static heap_bytes_s<u8> scratchBuffer(MAX_FRAME_SIZE, "Video recording conversion buffer.");

    k_assert(VIDEO_WRITER.isOpened(),
             "Attempted to record a video frame before video recording had been initialized.");

    // Get the current output frame.
    const resolution_s resolution = ks_output_resolution();
    const u8 *const frameData = ks_scaler_output_as_raw_ptr();
    if (frameData == nullptr) return;

    /// TODO: Force output padding when recording, so all frames have the same resolution.
    k_assert((resolution.w == RECORDING.resolution.w &&
              resolution.h == RECORDING.resolution.h), "Incompatible frame for recording: mismatched resolution.");

    // Convert the frame to BRG, and save it into the frame buffer.
    cv::Mat originalFrame(resolution.h, resolution.w, CV_8UC4, (u8*)frameData);
    cv::Mat frame = cv::Mat(resolution.h, resolution.w, CV_8UC3, RECORDING.activeFrameBuffer->next_slot());
    cv::cvtColor(originalFrame, frame, CV_BGRA2BGR);
    RECORDING.numFrames++;

   // DEBUG(("Frame rate: %.4f", (double)RECORDING.numFrames / (RECORDING.recordingTimer.elapsed() / 1000)));

    // Once we've accumulated enough frames to fill the frame buffer, encode
    // its contents into the video file.
    if (RECORDING.activeFrameBuffer->is_full())
    {
        // Run the encoding in a separate thread.
        const auto frameBuffer = RECORDING.activeFrameBuffer;
        RECORDING.encoderThread.waitForFinished();
        RECORDING.encoderThread = QtConcurrent::run([=]{encode_frame_buffer(frameBuffer);});

        // Meanwhile, switch to the other frame buffer, and keep accumulating new frames.
        RECORDING.flip_frame_buffer();
    }

    return;
}

void krecord_stop_recording(void)
{
    VIDEO_WRITER.release();

    return;
}
