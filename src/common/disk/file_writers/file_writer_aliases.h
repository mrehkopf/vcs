/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_WRITER_ALIASES_H
#define FILE_WRITER_ALIASES_H

#include "capture/alias.h"

namespace file_writer
{
namespace aliases
{
    namespace version_a
    {
        bool write(const std::string &filename,
                   const std::vector<mode_alias_s> &aliases);
    }
}
}

#endif
