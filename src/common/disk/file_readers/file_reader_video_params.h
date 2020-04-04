/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_READER_VIDEO_PARAMS_H
#define FILE_READER_VIDEO_PARAMS_H

#include "capture/capture.h"

namespace file_reader
{
namespace video_params
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
                  std::vector<video_signal_parameters_s> *const videoParams);
    }
}
}

#endif
