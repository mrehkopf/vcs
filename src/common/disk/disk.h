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
#include "display/display.h"

class BaseFilterGraphNode;
struct filter_graph_option_s;
struct video_signal_properties_s;
struct analog_video_preset_s;

bool kdisk_save_video_presets(const std::vector<analog_video_preset_s*> &presets, const std::string &filename);

bool kdisk_save_filter_graph(const std::vector<BaseFilterGraphNode*> &nodes, const std::string &filename);

std::vector<analog_video_preset_s*> kdisk_load_video_presets(const std::string &filename);

std::vector<abstract_filter_graph_node_s> kdisk_load_filter_graph(const std::string &filename);

#endif
