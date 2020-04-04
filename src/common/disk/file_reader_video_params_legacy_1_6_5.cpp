/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_reader_video_params.h"
#include "common/disk/csv.h"

bool file_reader::video_params::legacy_1_6_5::read(const std::string &filename,
                                                   std::vector<video_signal_parameters_s> *const videoParams)
{
    QList<QStringList> paramRows = csv_parse_c(QString::fromStdString(filename)).contents();

    if (paramRows.isEmpty())
    {
        NBENE(("Empty video parameters file."));
        goto fail;
    }

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int i = 0; i < paramRows.count();)
    {
        if ((paramRows.at(i).count() != 3) ||
            (paramRows.at(i).at(0) != "resolution"))
        {
            NBENE(("Expected a 3-parameter 'resolution' statement to begin a video parameters block."));
            goto fail;
        }

        video_signal_parameters_s p;
        p.r.w = paramRows.at(i).at(1).toUInt();
        p.r.h = paramRows.at(i).at(2).toUInt();

        i++;    // Move to the next row to start fetching the params for this resolution.

        auto get_param = [&](const QString &name)->QString
        {
            if ((int)i >= paramRows.length())
            {
                NBENE(("Error while loading video parameters: expected '%s' but found the data out of range.", name.toLatin1().constData()));
                throw 0;
            }
            else if (paramRows.at(i).at(0) != name)
            {
                NBENE(("Error while loading video parameters: expected '%s' but got '%s'.",
                    name.toLatin1().constData(), paramRows.at(i).at(0).toLatin1().constData()));
                throw 0;
            }
            return paramRows[i++].at(1);
        };
        
        try
        {
            // Note: the order in which the params are fetched is fixed to the
            // order in which they were saved.
            p.verticalPosition   = get_param("vPos").toInt();
            p.horizontalPosition = get_param("hPos").toInt();
            p.horizontalScale    = get_param("hScale").toUInt();
            p.phase              = get_param("phase").toInt();
            p.blackLevel         = get_param("bLevel").toInt();
            p.overallBrightness  = get_param("bright").toInt();
            p.overallContrast    = get_param("contr").toInt();
            p.redBrightness      = get_param("redBr").toInt();
            p.redContrast        = get_param("redCn").toInt();
            p.greenBrightness    = get_param("greenBr").toInt();
            p.greenContrast      = get_param("greenCn").toInt();
            p.blueBrightness     = get_param("blueBr").toInt();
            p.blueContrast       = get_param("blueCn").toInt();
        }
        catch (int)
        {
            NBENE(("Failed to load the video parameters from disk."));
            goto fail;
        }

        videoParams->push_back(p);
    }
    
    return true;

    fail:
    return false;
}
