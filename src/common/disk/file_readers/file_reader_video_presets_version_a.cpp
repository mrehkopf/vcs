/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_readers/file_reader_video_presets.h"
#include "common/disk/csv.h"

bool file_reader::video_presets::version_a::read(const std::string &filename,
                                                 std::vector<video_preset_s*> *const presets)
{
    // Bails out if the value (string) of the first cell on the current row doesn't match
    // the given one.
    #define FAIL_IF_FIRST_CELL_IS_NOT(string) if ((int)row >= rowData.length())\
                                              {\
                                                  NBENE(("Error while loading the video presets file: expected '%s' on line "\
                                                         "#%d but found the data out of range.", string, (row+1)));\
                                                  goto fail;\
                                              }\
                                              else if (rowData.at(row).at(0) != string)\
                                              {\
                                                  NBENE(("Error while loading the video presets file: expected '%s' on line "\
                                                         "#%d but found '%s' instead.",\
                                                         string, (row+1), rowData.at(row).at(0).toStdString().c_str()));\
                                                  goto fail;\
                                              }

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    if (rowData.isEmpty())
    {
        NBENE(("Empty video presets file."));
        goto fail;
    }

    // Load the data.
    {
        unsigned row = 0;

        FAIL_IF_FIRST_CELL_IS_NOT("fileType");
        //const QString fileType = rowData.at(row).at(1);

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("fileVersion");
        const QString fileVersion = rowData.at(row).at(1);
        k_assert((fileVersion == "a"), "Mismatched file version for reading.");
        if (fileVersion != "a")
        {
            NBENE(("Mismatched file version."));
            goto fail;
        }

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("presetCount");
        const int numPresets = rowData.at(row).at(1).toInt();
        if (numPresets < 0)
        {
            NBENE(("Negative preset counts are unsupported."));
            goto fail;
        }

        for (int i = 0; i < numPresets; i++)
        {
            video_preset_s *const preset = new video_preset_s;

            preset->id = unsigned(i);

            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("metadataCount");
            const int numMetadata = rowData.at(row).at(1).toInt();
            if (numMetadata < 0)
            {
                NBENE(("Negative metadata counts are unsupported."));
                goto fail;
            }

            for (int p = 0; p < numMetadata; p++)
            {
                row++;
                const auto metadataName = rowData.at(row).at(0);

                if (metadataName == "activatedByResolution")
                {
                    preset->activatesWithResolution = rowData.at(row).at(1).toInt();
                    preset->activationResolution.w = rowData.at(row).at(2).toInt();
                    preset->activationResolution.h = rowData.at(row).at(3).toInt();
                }
                else if (metadataName == "activatedByRefreshRate")
                {
                    preset->activatesWithRefreshRate = rowData.at(row).at(1).toInt();
                    preset->activationRefreshRate = rowData.at(row).at(3).toDouble();

                    const QString &comparator = rowData.at(row).at(2);
                    if      (comparator == "Equals")  preset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::equals;
                    else if (comparator == "Rounded") preset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::rounded;
                    else if (comparator == "Floored") preset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::floored;
                    else if (comparator == "Ceiled")  preset->refreshRateComparator = video_preset_s::refresh_rate_comparison_e::ceiled;
                    else
                    {
                        NBENE(("Unknown refresh rate comparator '%s'.", comparator.toStdString().c_str()));
                        goto fail;
                    }
                }
                else if (metadataName == "activatedByShortcut")
                {
                    preset->activatesWithShortcut = rowData.at(row).at(1).toInt();
                    preset->activationShortcut = rowData.at(row).at(2).toStdString();
                }
                else if (metadataName == "name")
                {
                    preset->name = rowData.at(row).at(1).toStdString();
                }
                else
                {
                    NBENE(("Unknown metadata '%s' in the video preset file. Ignoring it.",
                           metadataName.toStdString().c_str()));
                }
            }

            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("videoParameterCount");
            const int numParams = rowData.at(row).at(1).toInt();
            if (numParams < 0)
            {
                NBENE(("Negative parameter counts are unsupported."));
                goto fail;
            }

            for (int p = 0; p < numParams; p++)
            {
                row++;
                const auto paramName = rowData.at(row).at(0);

                if (paramName == "verticalPosition")
                {
                    preset->videoParameters.verticalPosition = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "horizontalPosition")
                {
                    preset->videoParameters.horizontalPosition = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "horizontalScale")
                {
                    preset->videoParameters.horizontalScale = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "phase")
                {
                    preset->videoParameters.phase = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blackLevel")
                {
                    preset->videoParameters.blackLevel = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "brightness")
                {
                    preset->videoParameters.overallBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "contrast")
                {
                    preset->videoParameters.overallContrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "redBrightness")
                {
                    preset->videoParameters.redBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "redContrast")
                {
                    preset->videoParameters.redContrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "greenBrightness")
                {
                    preset->videoParameters.greenBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "greenContrast")
                {
                    preset->videoParameters.greenContrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blueBrightness")
                {
                    preset->videoParameters.blueBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blueContrast")
                {
                    preset->videoParameters.blueContrast = rowData.at(row).at(1).toInt();
                }
                else
                {
                    NBENE(("Unknown parameter '%s' in the video preset file. Ignoring it.",
                           paramName.toStdString().c_str()));
                }
            }

            presets->push_back(preset);
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
