/*
 * 2019 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QTextStream>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QFile>
#include "display/qt/windows/ControlPanel/FilterGraph/BaseFilterGraphNode.h"
#include "common/disk/file_streamer.h"
#include "common/disk/file_reader.h"
#include "common/disk/file_writers/file_writer_video_presets.h"
#include "common/disk/file_readers/file_reader_video_presets.h"
#include "common/disk/file_readers/file_reader_filter_graph.h"
#include "common/disk/file_writers/file_writer_filter_graph.h"
#include "common/vcs_event/vcs_event.h"
#include "capture/capture.h"
#include "capture/video_presets.h"
#include "filter/filter.h"
#include "common/disk/disk.h"

bool kdisk_save_video_presets(const std::vector<analog_video_preset_s*> &presets,
                              const std::string &filename)
{
    if (!file_writer::video_presets::version_b::write(filename, presets))
    {
        goto fail;
    }

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the video parameters "
                                   "for saving. As a result, no data was saved. More information "
                                   "about this error may be found in the terminal.");
    return false;
}


std::vector<analog_video_preset_s*> kdisk_load_video_presets(const std::string &filename)
{
    if (filename.empty())
    {
        DEBUG(("No video presets file defined, skipping."));
        return {};
    }

    std::vector<analog_video_preset_s*> presets;
    const std::string fileVersion = file_reader::file_version(filename);

    if (fileVersion == "a")
    {
        if (!file_reader::video_presets::version_a::read(filename, &presets))
        {
            goto fail;
        }
    }
    else if (fileVersion == "b")
    {
        if (!file_reader::video_presets::version_b::read(filename, &presets))
        {
            goto fail;
        }
    }
    else
    {
        NBENE(("Unsupported video preset file format."));
        goto fail;
    }

    INFO(("Loaded %u video preset(s).", presets.size()));

    return presets;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading video presets. "
                                   "No data was loaded. More information about the error "
                                   "may be found in the terminal.");
    return {};
}

bool kdisk_save_filter_graph(const std::vector<BaseFilterGraphNode*> &nodes,
                             const std::string &filename)
{
    if (!file_writer::filter_graph::version_b::write(filename, nodes, {}))
    {
        goto fail;
    }

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing filter graph data for saving. "
                                   "As a result, no data was saved. More information about this "
                                   "error may be found in the terminal.");
    return false;
}

std::vector<abstract_filter_graph_node_s> kdisk_load_filter_graph(const std::string &filename)
{
    if (filename.empty())
    {
        DEBUG(("No filter graph file defined, skipping."));
        return {};
    }

    const std::string fileVersion = file_reader::file_version(filename);

    if (fileVersion.empty())
    {
        kd_show_headless_error_message("Unsupported filter graph file",
                                       "The filter graph file could not be loaded: its format is "
                                       "unsupported.");

        NBENE(("Unknown filter graph file format."));

        return {};
    }

    std::vector<abstract_filter_graph_node_s> graphNodes;

    if (fileVersion == "a")
    {
        if (!file_reader::filter_graph::version_a::read(filename, &graphNodes))
        {
            goto fail;
        }
    }
    else if (fileVersion == "b")
    {
        if (!file_reader::filter_graph::version_b::read(filename, &graphNodes))
        {
            goto fail;
        }
    }
    else
    {
        NBENE(("Unsupported filter graph file format."));
        goto fail;
    }

    INFO(("Loaded %u filter graph node(s).", graphNodes.size()));

    return graphNodes;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter graph. No data was "
                                   "loaded. More information about the error may be found in "
                                   "the terminal.");
    return {};
}
