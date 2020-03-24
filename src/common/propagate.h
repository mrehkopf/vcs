#ifndef PROPAGATE_H
#define PROPAGATE_H

#include <vector>
#include <string>

class FilterGraphNode;

struct filter_graph_option_s;
struct video_signal_parameters_s;
struct mode_alias_s;
struct resolution_s;

void kpropagate_news_of_new_capture_video_mode(void);
void kpropagate_news_of_invalid_capture_signal(void);
void kpropagate_news_of_gained_capture_signal(void);
void kpropagate_news_of_lost_capture_signal(void);
void kpropagate_news_of_unrecoverable_error(void);
void kpropagate_news_of_new_captured_frame(void);
void kpropagate_news_of_recording_started(void);
void kpropagate_news_of_recording_ended(void);

void kpropagate_saved_filter_graph_to_disk(const std::string &targetFilename);
void kpropagate_saved_video_signal_parameters_to_disk(const std::vector<video_signal_parameters_s> &p, const std::string &targetFilename);
void kpropagate_saved_aliases_to_disk(const std::vector<mode_alias_s> &aliases, const std::string &targetFilename);

void kpropagate_loaded_filter_graph_from_disk(const std::vector<FilterGraphNode*> &nodes,
                                              const std::vector<filter_graph_option_s> &graphOptions,
                                              const std::string &sourceFilename);
void kpropagate_loaded_video_signal_parameters_from_disk(const std::vector<video_signal_parameters_s> &p, const std::string &sourceFilename);
void kpropagate_loaded_aliases_from_disk(const std::vector<mode_alias_s> &aliases, const std::string &sourceFilename);

void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta);
void kpropagate_forced_capture_resolution(const resolution_s &r);

#endif
