/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/disk/file_writers/file_writer_video_params.h"
#include "common/disk/file_streamer.h"
#include "common/globals.h"

bool file_writer::video_params::version_a::write(const std::string &filename,
                                                 const std::vector<video_signal_properties_s> &videoParams)
{
    file_streamer_c outFile(filename);

    outFile << "fileType,{VCS video parameters}\n"
            << "fileVersion,a\n";

    outFile << "parameterSetCount," << videoParams.size() << "\n";

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &p: videoParams)
    {
        outFile << "parameterCount,14\n";

        // Resolution.
        outFile << "resolution," << p.r.w << "," << p.r.h << "\n";

        // Video params.
        outFile << "verticalPosition,"   << p.verticalPosition   << "\n"
                << "horizontalPosition," << p.horizontalPosition << "\n"
                << "horizontalScale,"    << p.horizontalSize    << "\n"
                << "phase,"              << p.phase              << "\n"
                << "blackLevel,"         << p.blackLevel         << "\n";

        // Color params.
        outFile << "brightness,"      << p.brightness << "\n"
                << "contrast,"        << p.contrast   << "\n"
                << "redBrightness,"   << p.redBrightness     << "\n"
                << "redContrast,"     << p.redContrast       << "\n"
                << "greenBrightness," << p.greenBrightness   << "\n"
                << "greenContrast,"   << p.greenContrast     << "\n"
                << "blueBrightness,"  << p.blueBrightness    << "\n"
                << "blueContrast,"    << p.blueContrast      << "\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to save video parameters."));
        goto fail;
    }

    return true;

    fail:
    return false;
}
