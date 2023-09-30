/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/disk/file_writers/file_writer_video_presets.h"
#include "common/disk/file_streamer.h"
#include "common/globals.h"

bool file_writer::video_presets::version_a::write(const std::string &filename,
                                                  const std::vector<analog_video_preset_s*> &presets)
{
    file_streamer_c outFile(filename);

    outFile << "fileType,{VCS video presets}\n"
            << "fileVersion,a\n";

    outFile << "presetCount," << presets.size() << "\n";

    for (const analog_video_preset_s *p: presets)
    {
        // Write the metadata.
        {
            outFile << "metadataCount,4\n";

            outFile << "name,{" << QString::fromStdString(p->name) << "}\n";

            outFile << "activatedByResolution," << p->activatesWithResolution << ","
                                                << p->activationResolution.w << ","
                                                << p->activationResolution.h << "\n";

            outFile << "activatedByRefreshRate," << p->activatesWithRefreshRate << ",";
            switch (p->refreshRateComparator)
            {
                case analog_video_preset_s::refresh_rate_comparison_e::equals:  outFile << "{Equals}"; break;
                case analog_video_preset_s::refresh_rate_comparison_e::ceiled:  outFile << "{Ceiled}"; break;
                case analog_video_preset_s::refresh_rate_comparison_e::floored: outFile << "{Floored}"; break;
                case analog_video_preset_s::refresh_rate_comparison_e::rounded: outFile << "{Rounded}"; break;
                default: k_assert(0, "Unhandled refresh rate comparator."); break;
            }
            outFile << "," << p->activationRefreshRate.value<double>() << "\n";

            outFile << "activatedByShortcut," << p->activatesWithShortcut << ","
                                              << QString::fromStdString(p->activationShortcut) << "\n";
        }

        // Write the video parameters.
        {
            outFile << "videoParameterCount,13\n";

            // Video params.
            outFile << "verticalPosition,"   << p->properties.verticalPosition   << "\n"
                    << "horizontalPosition," << p->properties.horizontalPosition << "\n"
                    << "horizontalScale,"    << p->properties.horizontalSize    << "\n"
                    << "phase,"              << p->properties.phase              << "\n"
                    << "blackLevel,"         << p->properties.blackLevel         << "\n";

            // Color params.
            outFile << "brightness,"      << p->properties.brightness << "\n"
                    << "contrast,"        << p->properties.contrast   << "\n"
                    << "redBrightness,"   << p->properties.redBrightness     << "\n"
                    << "redContrast,"     << p->properties.redContrast       << "\n"
                    << "greenBrightness," << p->properties.greenBrightness   << "\n"
                    << "greenContrast,"   << p->properties.greenContrast     << "\n"
                    << "blueBrightness,"  << p->properties.blueBrightness    << "\n"
                    << "blueContrast,"    << p->properties.blueContrast      << "\n";
        }
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to save video presets."));
        goto fail;
    }

    return true;

    fail:
    return false;
}
