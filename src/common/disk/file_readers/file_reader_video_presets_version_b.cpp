/*
 * 2020-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include <QKeySequence> /// TODO: Break dependency on Qt.
#include <regex>
#include "common/disk/file_readers/file_reader_video_presets.h"
#include "common/disk/csv.h"

bool file_reader::video_presets::version_b::read(
    const std::string &filename,
    std::vector<analog_video_preset_s*> *const presets
)
{
    // Bails out if the value (string) of the first cell on the current row doesn't match
    // the given one.
    #define FAIL_IF_FIRST_CELL_IS_NOT(string) \
        if ((int)row >= rowData.length())\
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
        k_assert((fileVersion == "b"), "Attempting to use an incorrect version of the video presets file reader.");

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
            analog_video_preset_s *const preset = new analog_video_preset_s;

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
                    if      (comparator == "Equals")  preset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::equals;
                    else if (comparator == "Rounded") preset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::rounded;
                    else if (comparator == "Floored") preset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::floored;
                    else if (comparator == "Ceiled")  preset->refreshRateComparator = analog_video_preset_s::refresh_rate_comparison_e::ceiled;
                    else
                    {
                        NBENE(("Unknown refresh rate comparator '%s'.", comparator.toStdString().c_str()));
                        goto fail;
                    }
                }
                else if (metadataName == "activatedByShortcut")
                {
                    preset->activatesWithShortcut = rowData.at(row).at(1).toInt();
                    preset->activationShortcut = QKeySequence(rowData.at(row).at(2)).toString().toStdString();

                    // Versions of VCS prior to 3.0.0 used "Ctrl+Fx" for preset activation,
                    // whereas version 3.0.0 uses "Fx". As a kludge fix to keep existing
                    // configurations valid, let's just silently convert.
                    preset->activationShortcut = std::regex_replace(
                        preset->activationShortcut,
                        std::regex("^Ctrl\\+"),
                        ""
                    );
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
            FAIL_IF_FIRST_CELL_IS_NOT("propertyCount");
            const int numParams = rowData.at(row).at(1).toInt();
            if (numParams < 0)
            {
                NBENE(("Negative parameter counts are unsupported."));
                goto fail;
            }

            for (int p = 0; p < numParams; p++)
            {
                const std::string paramName = rowData.at(++row).at(0).toStdString();
                preset->properties[paramName] = rowData.at(row).at(1).toInt();
            }

            presets->push_back(preset);
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
