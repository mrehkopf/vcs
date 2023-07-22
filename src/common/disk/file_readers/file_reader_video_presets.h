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

namespace file_reader::video_presets
{
    namespace version_a
    {
        bool read(const std::string &filename, std::vector<video_preset_s*> *const presets);
    }
}

#endif
