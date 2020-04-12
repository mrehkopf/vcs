/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/disk/file_writers/file_writer_aliases.h"
#include "common/disk/file_streamer.h"
#include "common/globals.h"
#include "filter/filter.h"

bool file_writer::aliases::version_a::write(const std::string &filename,
                                            const std::vector<mode_alias_s> &aliases)
{
    file_streamer_c outFile(filename);

    outFile << "fileType,{VCS aliases}\n"
            << "fileVersion,a\n";

    outFile << "aliasCount," << aliases.size() << "\n";

    for (const auto &a : aliases)
    {
        outFile << "alias," << a.from.w << "," << a.from.h << ","
                << "to," << a.to.w << "," << a.to.h << "\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to save aliases."));
        goto fail;
    }
    
    return true;

    fail:
    return false;
}
