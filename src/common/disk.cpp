/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS disk access
 *
 * Provides functions for loading and saving data to and from files on disk.
 *
 */

#include <QTextStream>
#include <QString>
#include <QFile>
#include "../capture/capture.h"
#include "../common/propagate.h"
#include "../common/csv.h"
#include "disk.h"

// A wrapper for Qt's file-saving functionality. Streams given strings via the
// << operator on the object into a file.
class file_writer_c
{
public:
    file_writer_c(QString filename) :
        targetFilename(filename),
        tempFilename(targetFilename + ".temporary")
    {
        file.setFileName(tempFilename);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        stream.setDevice(&file);

        return;
    }

    // Saves the data from the temp file to the final target file.
    bool save(void)
    {
        if (QFile(targetFilename).exists() &&
            !QFile(targetFilename).remove())
        {
            return false;
        }

        if (!QFile(tempFilename).rename(targetFilename))
        {
            return false;
        }

        return true;
    }

    bool is_valid(void)
    {
        return isValid;
    }

    QTextStream& operator<<(const QString &string)
    {
        if (this->isValid)
        {
            stream << string;
            test_validity();
        }

        return stream;
    }

    QTextStream& operator<<(const QChar &character)
    {
        return stream << QString(character);
    }

    QTextStream& operator<<(const unsigned long ulong)
    {
        return stream << QString::number(ulong);
    }

private:
    bool isValid = true;

    QFile file;
    QTextStream stream;

    // The filename we want to save to.
    QString targetFilename;

    // The filename we'll first temporarily write the data to. If all goes well,
    // we'll rename this to the target filename, and remove the old target file,
    // if it exists.
    QString tempFilename;

    void test_validity(void)
    {
        if (!this->file.isWritable() ||
            !this->file.isOpen() ||
            this->file.error() != QFileDevice::NoError)
        {
            isValid = false;
        }

        return;
    }
};

bool kdisk_save_mode_params(const std::vector<mode_params_s> &modeParams,
                         const QString &targetFilename)
{
    file_writer_c outFile(targetFilename);

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &m: modeParams)
    {
        // Resolution.
        outFile << "resolution," << m.r.w << ',' << m.r.h << '\n';

        // Video params.
        outFile << "vPos," << m.video.verticalPosition << '\n'
                << "hPos," << m.video.horizontalPosition << '\n'
                << "hScale," << m.video.horizontalScale << '\n'
                << "phase," << m.video.phase << '\n'
                << "bLevel," << m.video.blackLevel << '\n';

        // Color params.
        outFile << "bright," << m.color.overallBrightness << '\n'
                << "contr," << m.color.overallContrast << '\n'
                << "redBr," << m.color.redBrightness << '\n'
                << "redCn," << m.color.redContrast << '\n'
                << "greenBr," << m.color.greenBrightness << '\n'
                << "greenCn," << m.color.greenContrast << '\n'
                << "blueBr," << m.color.blueBrightness << '\n'
                << "blueCn," << m.color.blueContrast << '\n';

        // Separate the next block.
        outFile << '\n';
    }

    if (!outFile.is_valid() ||
        !outFile.save())
    {
        NBENE(("Failed to write mode params to file."));
        goto fail;
    }

    kpropagate_saved_mode_params_to_disk(modeParams, targetFilename.toStdString());

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the mode "
                                   "settings for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_load_mode_params(const QString &sourceFilename)
{
    if (sourceFilename.isEmpty())
    {
        DEBUG(("No mode settings file defined, skipping."));
        return true;
    }

    std::vector<mode_params_s> modeParams;

    QList<QStringList> paramRows = csv_parse_c(sourceFilename).contents();

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int i = 0; i < paramRows.count();)
    {
        if ((paramRows[i].count() != 3) ||
            (paramRows[i].at(0) != "resolution"))
        {
            NBENE(("Expected a 3-parameter 'resolution' statement to begin a mode params block."));
            goto fail;
        }

        mode_params_s p;
        p.r.w = paramRows[i].at(1).toUInt();
        p.r.h = paramRows[i].at(2).toUInt();

        i++;    // Move to the next row to start fetching the params for this resolution.

        auto get_param = [&](const QString &name)->QString
                         {
                             if (paramRows[i].at(0) != name)
                             {
                                 NBENE(("Error while loading the mode params file: expected '%s' but got '%s'.",
                                        name.toLatin1().constData(), paramRows[i].at(0).toLatin1().constData()));
                                 throw 0;
                             }
                             return paramRows[i++].at(1);
                         };
        try
        {
            // Note: the order in which the params are fetched is fixed to the
            // order in which they were saved.
            p.video.verticalPosition = get_param("vPos").toInt();
            p.video.horizontalPosition = get_param("hPos").toInt();
            p.video.horizontalScale = get_param("hScale").toInt();
            p.video.phase = get_param("phase").toInt();
            p.video.blackLevel = get_param("bLevel").toInt();
            p.color.overallBrightness = get_param("bright").toInt();
            p.color.overallContrast = get_param("contr").toInt();
            p.color.redBrightness = get_param("redBr").toInt();
            p.color.redContrast = get_param("redCn").toInt();
            p.color.greenBrightness = get_param("greenBr").toInt();
            p.color.greenContrast = get_param("greenCn").toInt();
            p.color.blueBrightness = get_param("blueBr").toInt();
            p.color.blueContrast = get_param("blueCn").toInt();
        }
        catch (int)
        {
            NBENE(("Failed to load mode params from disk."));
            goto fail;
        }

        modeParams.push_back(p);
    }

    // Sort the modes so they'll display more nicely in the GUI.
    std::sort(modeParams.begin(), modeParams.end(), [](const mode_params_s &a, const mode_params_s &b)
                                          { return (a.r.w * a.r.h) < (b.r.w * b.r.h); });

    kpropagate_loaded_mode_params_from_disk(modeParams, sourceFilename.toStdString());

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the mode "
                                   "parameter file. No data was loaded.\n\nMore "
                                   "information about the error may be found in the terminal.");
    return false;
}

// Loads alias definitions from the given file. Will expect automaticCall to be
// set to false if this function was called directly by a request by the user
// through the GUI to load the aliases (as opposed to being called automatically
// on startup or so).
//
bool kdisk_load_aliases(const QString &sourceFilename)
{
    if (sourceFilename.isEmpty())
    {
        DEBUG(("No alias file defined, skipping."));
        return true;
    }

    std::vector<mode_alias_s> aliases;

    QList<QStringList> rowData = csv_parse_c(sourceFilename).contents();
    for (const auto &row: rowData)
    {
        if (row.count() != 4)
        {
            NBENE(("Expected a 4-parameter row in the alias file."));
            goto fail;
        }

        mode_alias_s a;
        a.from.w = row.at(0).toUInt();
        a.from.h = row.at(1).toUInt();
        a.to.w = row.at(2).toUInt();
        a.to.h = row.at(3).toUInt();

        aliases.push_back(a);
    }

    // Sort the parameters so they display more nicely in the GUI.
    std::sort(aliases.begin(), aliases.end(), [](const mode_alias_s &a, const mode_alias_s &b)
                                              { return (a.to.w * a.to.h) < (b.to.w * b.to.h); });

    kpropagate_loaded_aliases_from_disk(aliases, sourceFilename.toStdString());

    // Signal a new input mode to force the program to re-evaluate the mode
    // parameters, in case one of the newly-loaded aliases applies to the
    // current mode.
    kpropagate_news_of_new_capture_video_mode();

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the alias "
                                   "file. No data was loaded.\n\nMore "
                                   "information about the error may be found in the terminal.");
    return false;
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const QString &targetFilename)
{
    file_writer_c outFile(targetFilename);

    for (const auto &a: aliases)
    {
        outFile << a.from.w << ',' << a.from.h << ','
                << a.to.w << ',' << a.to.h << ",\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save())
    {
        NBENE(("Failed to write the aliases to file."));
        goto fail;
    }

    kpropagate_saved_aliases_to_disk(aliases, targetFilename.toStdString());

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the alias "
                                   "resolutions for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}
