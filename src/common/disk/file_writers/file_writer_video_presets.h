/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_WRITER_VIDEO_PRESETS_H
#define FILE_WRITER_VIDEO_PRESETS_H

#include "capture/video_presets.h"

namespace file_writer
{
namespace video_presets
{
    // Legacy for VCS <= 1.6.5.
    namespace legacy_1_6_5
    {
        bool write(const std::string &filename,
                   const std::vector<video_signal_parameters_s> &videoParams);
    }

    namespace version_a
    {
        bool write(const std::string &filename,
                   const std::vector<video_preset_s*> &presets);
    }
}
}

#endif
