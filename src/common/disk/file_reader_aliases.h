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
    namespace version_a
    {
        bool read(const std::string &filename,
                  std::vector<mode_alias_s> *const aliases);
    }
}
}

#endif
