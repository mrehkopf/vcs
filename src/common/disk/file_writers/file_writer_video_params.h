/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_VIDEO_PARAMS_H
#define VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_VIDEO_PARAMS_H

#include "capture/capture.h"

namespace file_writer::video_params
{
    namespace version_a
    {
        bool write(const std::string &filename, const std::vector<video_signal_parameters_s> &videoParams);
    }
}

#endif
