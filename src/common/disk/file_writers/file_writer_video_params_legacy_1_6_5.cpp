/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/disk/file_writers/file_writer_video_params.h"
#include "common/disk/file_streamer.h"
#include "common/globals.h"

bool file_writer::video_params::legacy_1_6_5::write(const std::string &filename,
                                                    const std::vector<video_signal_parameters_s> &videoParams)
{
    file_streamer_c outFile(filename);

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &p: videoParams)
    {
        // Resolution.
        outFile << "resolution," << p.r.w << "," << p.r.h << "\n";

        // Video params.
        outFile << "vPos,"   << p.verticalPosition << "\n"
                << "hPos,"   << p.horizontalPosition << "\n"
                << "hScale," << p.horizontalScale << "\n"
                << "phase,"  << p.phase << "\n"
                << "bLevel," << p.blackLevel << "\n";

        // Color params.
        outFile << "bright,"  << p.overallBrightness << "\n"
                << "contr,"   << p.overallContrast << "\n"
                << "redBr,"   << p.redBrightness << "\n"
                << "redCn,"   << p.redContrast << "\n"
                << "greenBr," << p.greenBrightness << "\n"
                << "greenCn," << p.greenContrast << "\n"
                << "blueBr,"  << p.blueBrightness << "\n"
                << "blueCn,"  << p.blueContrast << "\n";

        // Separate the next block.
        outFile << "\n";
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
