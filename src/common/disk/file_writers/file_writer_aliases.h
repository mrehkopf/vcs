/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_ALIASES_H
#define VCS_COMMON_DISK_FILE_WRITERS_FILE_WRITER_ALIASES_H

#include "capture/alias.h"

namespace file_writer
{
namespace aliases
{
    namespace version_a
    {
        bool write(const std::string &filename,
                   const std::vector<resolution_alias_s> &aliases);
    }
}
}

#endif
