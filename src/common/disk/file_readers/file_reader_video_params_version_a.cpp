/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_readers/file_reader_video_params.h"
#include "common/disk/csv.h"

bool file_reader::video_params::version_a::read(const std::string &filename,
                                                std::vector<video_signal_parameters_s> *const videoParams)
{
    // Bails out if the value (string) of the first cell on the current row doesn't match
    // the given one.
    #define FAIL_IF_FIRST_CELL_IS_NOT(string) if ((int)row >= rowData.length())\
                                              {\
                                                  NBENE(("Error while loading the filter graph file: expected '%s' on line "\
                                                         "#%d but found the data out of range.", string, (row+1)));\
                                                  goto fail;\
                                              }\
                                              else if (rowData.at(row).at(0) != string)\
                                              {\
                                                  NBENE(("Error while loading filter graph file: expected '%s' on line "\
                                                         "#%d but found '%s' instead.",\
                                                         string, (row+1), rowData.at(row).at(0).toStdString().c_str()));\
                                                  goto fail;\
                                              }

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    if (rowData.isEmpty())
    {
        NBENE(("Empty filter graph file."));
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

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("parameterSetCount");
        const unsigned numParameterSets = rowData.at(row).at(1).toUInt();

        for (unsigned i = 0; i < numParameterSets; i++)
        {
            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("parameterCount");
            const unsigned numParameters = rowData.at(row).at(1).toUInt();

            video_signal_parameters_s params;

            for (unsigned p = 0; p < numParameters; p++)
            {
                row++;
                const auto paramName = rowData.at(row).at(0);

                if (paramName == "resolution")
                {
                    params.r.w = rowData.at(row).at(1).toInt();
                    params.r.h = rowData.at(row).at(2).toInt();
                }
                else if (paramName == "verticalPosition")
                {
                    params.verticalPosition = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "horizontalPosition")
                {
                    params.horizontalPosition = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "horizontalScale")
                {
                    params.horizontalSize = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "phase")
                {
                    params.phase = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blackLevel")
                {
                    params.blackLevel = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "brightness")
                {
                    params.brightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "contrast")
                {
                    params.contrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "redBrightness")
                {
                    params.redBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "redContrast")
                {
                    params.redContrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "greenBrightness")
                {
                    params.greenBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "greenContrast")
                {
                    params.greenContrast = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blueBrightness")
                {
                    params.blueBrightness = rowData.at(row).at(1).toInt();
                }
                else if (paramName == "blueContrast")
                {
                    params.blueContrast = rowData.at(row).at(1).toInt();
                }
                else
                {
                    NBENE(("Unknown parameter %s in the video parameters file. Ignoring it.",
                           paramName.toStdString().c_str()));
                }
            }

            videoParams->push_back(params);
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
