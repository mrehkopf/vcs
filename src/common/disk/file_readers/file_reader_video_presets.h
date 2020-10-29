/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_READERS_FILE_READER_VIDEO_PRESETS_H
#define VCS_COMMON_DISK_FILE_READERS_FILE_READER_VIDEO_PRESETS_H

#include "capture/capture.h"
#include "capture/video_presets.h"

namespace file_reader
{
namespace video_presets
{
    // Legacy for VCS <= 1.6.5.
    namespace legacy_1_6_5
    {
        bool read(const std::string &filename,
                  std::vector<video_signal_parameters_s> *const videoParams);
    }

    namespace version_a
    {
        bool read(const std::string &filename,
                  std::vector<video_preset_s*> *const presets);
    }
}
}

#endif
