/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_READER_H
#define FILE_READER_H

#include <string>

namespace file_reader
{
    // Returns the file version of the given file, or an empty string "" if the
    // version can't be determined.
    std::string file_version(const std::string &filename);
}

#endif
