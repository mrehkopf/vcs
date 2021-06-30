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
#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "common/disk/file_streamer.h"
#include "common/disk/file_reader.h"
#include "common/disk/file_writers/file_writer_video_presets.h"
#include "common/disk/file_readers/file_reader_video_presets.h"
#include "common/disk/file_readers/file_reader_video_params.h"
#include "common/disk/file_writers/file_writer_video_params.h"
#include "common/disk/file_readers/file_reader_filter_graph.h"
#include "common/disk/file_writers/file_writer_filter_graph.h"
#include "common/disk/file_writers/file_writer_aliases.h"
#include "common/disk/file_readers/file_reader_aliases.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "capture/video_presets.h"
#include "capture/alias.h"
#include "filter/filter.h"
#include "common/disk/disk.h"

vcs_event_c<void> kdisk_evSavedVideoPresets;
vcs_event_c<void> kdisk_evSavedFilterGraph;
vcs_event_c<void> kdisk_evSavedAliases;
vcs_event_c<void> kdisk_evLoadedVideoPresets;
vcs_event_c<void> kdisk_evLoadedFilterGraph;
vcs_event_c<void> kdisk_evLoadedAliases;

bool kdisk_save_video_presets(const std::vector<video_preset_s*> &presets,
                              const std::string &targetFilename)
{
    if (!file_writer::video_presets::version_a::write(targetFilename, presets))
    {
        goto fail;
    }

    kdisk_evSavedVideoPresets.fire();

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the video parameters "
                                   "for saving. As a result, no data was saved. More information "
                                   "about this error may be found in the terminal.");
    return false;
}


std::vector<video_preset_s*> kdisk_load_video_presets(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No video presets file defined, skipping."));
        return {};
    }

    std::vector<video_preset_s*> presets;

    const std::string fileVersion = file_reader::file_version(sourceFilename);
    const std::string fileType = file_reader::file_type(sourceFilename);

    // Legacy parameter file format (VCS <= 1.6.5).
    if (fileVersion.empty())
    {
        std::vector<video_signal_parameters_s> videoModeParams;

        if (!file_reader::video_params::legacy_1_6_5::read(sourceFilename, &videoModeParams))
        {
            goto fail;
        }

        // Convert the legacy format into the current format.
        unsigned id = 0;
        for (const auto &params: videoModeParams)
        {
            video_preset_s *const preset = new video_preset_s;

            preset->activatesWithResolution = true;
            preset->activationResolution = {params.r.w, params.r.h, 0};
            preset->videoParameters = params;
            preset->id = ++id;

            presets.push_back(preset);
        }
    }
    // Legacy parameter file format (VCS ~1.7-dev).
    else if (fileType == "VCS video parameters")
    {
        std::vector<video_signal_parameters_s> videoModeParams;

        if (!file_reader::video_params::version_a::read(sourceFilename, &videoModeParams))
        {
            goto fail;
        }

        // Convert the legacy format into the current format.
        unsigned id = 0;
        for (const auto &params: videoModeParams)
        {
            video_preset_s *const preset = new video_preset_s;

            preset->activatesWithResolution = true;
            preset->activationResolution = {params.r.w, params.r.h, 0};
            preset->videoParameters = params;
            preset->id = ++id;

            presets.push_back(preset);
        }
    }
    // Current file format (VCS >= 2.0.0).
    else
    {
        if (fileVersion == "a")
        {
            if (!file_reader::video_presets::version_a::read(sourceFilename, &presets))
            {
                goto fail;
            }
        }
        else
        {
            NBENE(("Unsupported video preset file format."));
            goto fail;
        }
    }

    kdisk_evLoadedVideoPresets.fire();

    INFO(("Loaded %u video preset(s).", presets.size()));

    return presets;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading video presets. "
                                   "No data was loaded. More information about the error "
                                   "may be found in the terminal.");
    return {};
}

std::vector<mode_alias_s> kdisk_load_aliases(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No alias file defined, skipping."));
        return {};
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

    kdisk_evLoadedAliases.fire();

    INFO(("Loaded %u alias(es).", aliases.size()));

    return aliases;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading aliases. No data was "
                                   "loaded. More information about the error may be found in "
                                   "the terminal.");
    return {};
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const std::string &targetFilename)
{
    if (!file_writer::aliases::version_a::write(targetFilename, aliases))
    {
        goto fail;
    }

    kdisk_evSavedAliases.fire();

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the alias "
                                   "resolutions for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_save_filter_graph(std::vector<FilterGraphNode*> &nodes,
                             const std::string &targetFilename)
{
    if (!file_writer::filter_graph::version_b::write(targetFilename, nodes, {}))
    {
        goto fail;
    }

    kdisk_evSavedFilterGraph.fire();

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing filter graph data for saving. "
                                   "As a result, no data was saved. More information about this "
                                   "error may be found in the terminal.");
    return false;
}

std::vector<FilterGraphNode*> kdisk_load_filter_graph(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No filter graph file defined, skipping."));
        return {};
    }

    const std::string fileVersion = file_reader::file_version(sourceFilename);

    if (fileVersion.empty())
    {
        kd_show_headless_error_message("Unsupported filter graph file",
                                       "The filter graph file could not be loaded: its format is "
                                       "unsupported.");

        NBENE(("Unknown filter graph file format."));

        return {};
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

    kdisk_evLoadedFilterGraph.fire();

    INFO(("Loaded %u filter graph node(s).", graphNodes.size()));

    return graphNodes;

    fail:
    kd_clear_filter_graph();
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter graph. No data was "
                                   "loaded. More information about the error may be found in "
                                   "the terminal.");
    return {};
}
