/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_VIDEO_PRESETS_H
#define VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_VIDEO_PRESETS_H

#include "capture/video_presets.h"

namespace file_writer::video_presets
{
    namespace version_b
    {
        bool write(const std::string &filename, const std::vector<analog_video_preset_s*> &presets);
    }
}

#endif
