/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_READER_ALIASES_H
#define FILE_READER_ALIASES_H

#include "capture/alias.h"

namespace file_reader
{
namespace aliases
{
    // Legacy for VCS <= 1.6.5.
    namespace legacy_1_6_5
    {
        bool read(const std::string &filename,
                  std::vector<resolution_alias_s> *const aliases);
    }

    namespace version_a
    {
        bool read(const std::string &filename,
                  std::vector<resolution_alias_s> *const aliases);
    }
}
}

#endif
