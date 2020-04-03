/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS disk access
 *
 * Provides functions for loading and saving data to and from files on disk.
 *
 */

#include <QTextStream>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QFile>
#include "display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.h"
#include "display/qt/widgets/filter_widgets.h"
#include "common/disk/file_writer.h"
#include "common/disk/file_reader.h"
#include "common/disk/file_reader_filter_graph.h"
#include "common/disk/file_writer_filter_graph.h"
#include "common/propagate/propagate.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "filter/filter.h"
#include "common/disk/disk.h"
#include "common/disk/csv.h"

bool kdisk_save_video_signal_parameters(const std::vector<video_signal_parameters_s> &params,
                                        const std::string &targetFilename)
{
    file_writer_c outFile(targetFilename);

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &p: params)
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
        NBENE(("Failed to write mode params to file."));
        goto fail;
    }

    kpropagate_saved_video_signal_parameters_to_disk(params, targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the mode "
                                   "settings for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_load_video_signal_parameters(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No mode settings file defined, skipping."));
        return true;
    }

    DEBUG(("Loading video mode parameters from %s...", sourceFilename.c_str()));

    std::vector<video_signal_parameters_s> videoModeParams;

    QList<QStringList> paramRows = csv_parse_c(QString::fromStdString(sourceFilename)).contents();

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int i = 0; i < paramRows.count();)
    {
        if ((paramRows.at(i).count() != 3) ||
            (paramRows.at(i).at(0) != "resolution"))
        {
            NBENE(("Expected a 3-parameter 'resolution' statement to begin a mode params block."));
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
            NBENE(("Failed to load mode params from disk."));
            goto fail;
        }

        videoModeParams.push_back(p);
    }

    // Sort the modes so they'll display more nicely in the GUI.
    std::sort(videoModeParams.begin(), videoModeParams.end(), [](const video_signal_parameters_s &a, const video_signal_parameters_s &b)
                                          { return (a.r.w * a.r.h) < (b.r.w * b.r.h); });

    kpropagate_loaded_video_signal_parameters_from_disk(videoModeParams, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading video parameters. "
                                   "No data was loaded.\n\nMore information about the error "
                                   "may be found in the terminal.");
    return false;
}

bool kdisk_load_aliases(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No alias file defined, skipping."));
        return true;
    }

    DEBUG(("Loading aliases from %s...", sourceFilename.c_str()));

    std::vector<mode_alias_s> aliases;

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(sourceFilename)).contents();
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

    kpropagate_loaded_aliases_from_disk(aliases, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading aliases. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const std::string &targetFilename)
{
    file_writer_c outFile(targetFilename);

    for (const auto &a: aliases)
    {
        outFile << a.from.w << "," << a.from.h << ","
                << a.to.w << "," << a.to.h << ",\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write aliases to file."));
        goto fail;
    }

    kpropagate_saved_aliases_to_disk(aliases, targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the alias "
                                   "resolutions for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_save_filter_graph(std::vector<FilterGraphNode*> &nodes,
                             std::vector<filter_graph_option_s> &graphOptions,
                             const std::string &targetFilename)
{
    if (!file_writer::filter_graph::version_b::write(targetFilename, nodes, graphOptions))
    {
        goto fail;
    }

    kpropagate_saved_filter_graph_to_disk(targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing filter graph data for saving. "
                                   "As a result, no data was saved. \n\nMore information about this "
                                   "error may be found in the terminal.");
    return false;
}

bool kdisk_load_filter_graph(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No filter graph file defined, skipping."));
        return true;
    }

    const std::string fileVersion = file_reader::file_version(sourceFilename);

    if (fileVersion.empty())
    {
        kd_show_headless_error_message("Unsupported filter graph file",
                                       "The filter graph file could not be loaded: its format is "
                                       "unsupported.");

        NBENE(("Unknown filter graph file format."));

        return false;
    }

    std::vector<FilterGraphNode*> graphNodes;
    std::vector<filter_graph_option_s> graphOptions;

    kd_clear_filter_graph();

    if (fileVersion == "a")
    {
        if (!file_reader::filter_graph::version_a::read(sourceFilename, &graphNodes, &graphOptions))
        {
            NBENE(("Unknown filter graph file format."));
            goto fail;
        }
    }
    else if (fileVersion == "b")
    {
        if (!file_reader::filter_graph::version_b::read(sourceFilename, &graphNodes, &graphOptions))
        {
            NBENE(("Unknown filter graph file format."));
            goto fail;
        }
    }
    else
    {
        NBENE(("Unsupported filter graph file format."));
        goto fail;
    }

    kpropagate_loaded_filter_graph_from_disk(graphNodes, graphOptions, sourceFilename);

    return true;

    fail:
    kd_clear_filter_graph();
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter graph. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}
