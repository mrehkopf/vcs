#ifndef VCS_COMMON_DISK_DISK_H
#define VCS_COMMON_DISK_DISK_H

#include <vector>
#include <string>
#include "common/propagate/app_events.h"

class FilterGraphNode;
class QString;

struct filter_graph_option_s;
struct video_signal_parameters_s;
struct video_preset_s;
struct mode_alias_s;

extern app_event_c<void> kdiskEvent_savedVideoPresets;
extern app_event_c<void> kdiskEvent_savedFilterGraph;
extern app_event_c<void> kdiskEvent_savedAliases;
extern app_event_c<void> kdiskEvent_loadedVideoPresets;
extern app_event_c<void> kdiskEvent_loadedFilterGraph;
extern app_event_c<void> kdiskEvent_loadedAliases;

bool kdisk_save_video_presets(const std::vector<video_preset_s*> &presets, const std::string &targetFilename);
bool kdisk_save_filter_graph(std::vector<FilterGraphNode*> &nodes, std::vector<filter_graph_option_s> &options, const std::string &targetFilename);
bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases, const std::string &targetFilename);

std::vector<video_preset_s*> kdisk_load_video_presets(const std::string &sourceFilename);
std::pair<std::vector<FilterGraphNode*>,
          std::vector<filter_graph_option_s>> kdisk_load_filter_graph(const std::string &sourceFilename);
std::vector<mode_alias_s> kdisk_load_aliases(const std::string &sourceFilename);

#endif
