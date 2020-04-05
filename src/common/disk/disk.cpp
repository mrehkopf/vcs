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
#include "display/qt/widgets/filter_widgets.h"
#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "common/disk/file_streamer.h"
#include "common/disk/file_reader.h"
#include "common/disk/file_readers/file_reader_video_params.h"
#include "common/disk/file_writers/file_writer_video_params.h"
#include "common/disk/file_readers/file_reader_filter_graph.h"
#include "common/disk/file_writers/file_writer_filter_graph.h"
#include "common/disk/file_writers/file_writer_aliases.h"
#include "common/disk/file_readers/file_reader_aliases.h"
#include "common/propagate/propagate.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "filter/filter.h"
#include "common/disk/disk.h"

bool kdisk_save_video_signal_parameters(const std::vector<video_signal_parameters_s> &params,
                                        const std::string &targetFilename)
{
    if (!file_writer::video_params::version_a::write(targetFilename, params))
    {
        goto fail;
    }

    kpropagate_saved_video_signal_parameters_to_disk(params, targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the video parameters "
                                   "for saving. As a result, no data was saved. More information "
                                   "about this error may be found in the terminal.");
    return false;
}

bool kdisk_load_video_signal_parameters(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No mode settings file defined, skipping."));
        return true;
    }

    std::vector<video_signal_parameters_s> videoModeParams;

    const std::string fileVersion = file_reader::file_version(sourceFilename);

    if (fileVersion.empty())
    {
        if (!file_reader::video_params::legacy_1_6_5::read(sourceFilename, &videoModeParams))
        {
            goto fail;
        }
    }
    else if (fileVersion == "a")
    {
        if (!file_reader::video_params::version_a::read(sourceFilename, &videoModeParams))
        {
            goto fail;
        }
    }
    else
    {
        NBENE(("Unsupported video signal parameter file format."));
        goto fail;
    }

    // Sort the modes so they'll display more nicely in the GUI.
    std::sort(videoModeParams.begin(), videoModeParams.end(), [](const video_signal_parameters_s &a, const video_signal_parameters_s &b)
    {
        return (a.r.w * a.r.h) < (b.r.w * b.r.h);
    });

    kpropagate_loaded_video_signal_parameters_from_disk(videoModeParams, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading video parameters. "
                                   "No data was loaded. More information about the error "
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

    const std::string fileVersion = file_reader::file_version(sourceFilename);

    std::vector<mode_alias_s> aliases;

    if (fileVersion.empty())
    {
        if (!file_reader::aliases::legacy_1_6_5::read(sourceFilename, &aliases))
        {
            goto fail;
        }
    }
    else if (fileVersion == "a")
    {
        if (!file_reader::aliases::version_a::read(sourceFilename, &aliases))
        {
            goto fail;
        }
    }
    else
    {
        NBENE(("Unsupported alias file format."));
        goto fail;
    }

    // Sort the aliases so they display more nicely in the GUI.
    std::sort(aliases.begin(), aliases.end(), [](const mode_alias_s &a, const mode_alias_s &b)
    {
        return (a.to.w * a.to.h) < (b.to.w * b.to.h);
    });

    kpropagate_loaded_aliases_from_disk(aliases, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading aliases. No data was "
                                   "loaded. More information about the error may be found in "
                                   "the terminal.");
    return false;
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const std::string &targetFilename)
{
    if (!file_writer::aliases::version_a::write(targetFilename, aliases))
    {
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
                                   "As a result, no data was saved. More information about this "
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
            goto fail;
        }
    }
    else if (fileVersion == "b")
    {
        if (!file_reader::filter_graph::version_b::read(sourceFilename, &graphNodes, &graphOptions))
        {
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
                                   "loaded. More information about the error may be found in "
                                   "the terminal.");
    return false;
}
