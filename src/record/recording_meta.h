/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_RECORD_RECORDING_META_H
#define VCS_RECORD_RECORDING_META_H

#include <future>
#include <string>
#include <QElapsedTimer>
#include "common/globals.h"

struct recording_meta_s
{
    std::future<bool> encoderThreadFuture;

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

    // The number of frames the capture subsystem has missed. We'll use this
    // value to keep track of dropped frames on the capture-side.
    uint numGlobalDroppedFrames;

    // The maximum recoring buffer usage, in percent, seen during the
    // recording.
    uint peakBufferUsagePercent;

    // Milliseconds passed since the recording was started.
    QElapsedTimer recordingTimer;
};

#endif
