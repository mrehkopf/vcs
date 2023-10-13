/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

/*
 * The disk subsystem interface.
 * 
 * The disk subsystem provides functionality to VCS for saving and loading user
 * data files -- e.g. video preset configurations.
 * 
 * The subsystem transparently handles loading from legacy file formats.
 *
 */

#ifndef VCS_COMMON_DISK_DISK_H
#define VCS_COMMON_DISK_DISK_H

#include <vector>
#include <string>
#include "common/vcs_event/vcs_event.h"
#include "filter/abstract_filter.h"

class BaseFilterGraphNode;
struct filter_graph_option_s;
struct analog_properties_s;
struct analog_video_preset_s;

// A GUI-agnostic representation of a filter graph node.
//
// Used to mediate data between the disk subsystem's file loader and the display
// subsystem's (GUI framework-dependent) filter graph implementation, so that the
// file loader can remain independent from the GUI framework.
struct abstract_filter_graph_node_s
{
    // Uniquely identifies an instance of this node from other instances.
    int id = 0;

    // Whether this node is active (true) or a passthrough that applies no processing (false).
    bool isEnabled = true;

    // The UUID of the filter type that this node represents (see abstract_filter_c).
    std::string typeUuid = "";

    // The color of the node's background in the filter graph.
    std::string backgroundColor = "black";

    // The initial parameters of the filter associated with this node.
    filter_params_t initialParameters;

    // The node's initial XY coordinates in the filter graph.
    std::pair<double, double> initialPosition = {0, 0};

    // The nodes (identified by their @ref id) to which this node is connected.
    std::vector<int> connectedTo;
};

bool kdisk_save_video_presets(const std::vector<analog_video_preset_s*> &presets, const std::string &filename);

bool kdisk_save_filter_graph(const std::vector<BaseFilterGraphNode*> &nodes, const std::string &filename);

std::vector<analog_video_preset_s*> kdisk_load_video_presets(const std::string &filename);

std::vector<abstract_filter_graph_node_s> kdisk_load_filter_graph(const std::string &filename);

#endif
