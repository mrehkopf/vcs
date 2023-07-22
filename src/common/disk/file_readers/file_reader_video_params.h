/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_READERS_FILE_READER_VIDEO_PARAMS_H
#define VCS_COMMON_DISK_FILE_READERS_FILE_READER_VIDEO_PARAMS_H

#include "capture/capture.h"

namespace file_reader::video_params
{
    namespace version_a
    {
        bool read(const std::string &filename, std::vector<video_signal_parameters_s> *const videoParams);
    }
}

#endif
