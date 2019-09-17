#ifndef PROPAGATE_H
#define PROPAGATE_H

#include <vector>
#include <string>

struct video_mode_params_s;
struct mode_alias_s;
struct resolution_s;

void kpropagate_news_of_new_capture_video_mode(void);
void kpropagate_news_of_invalid_capture_signal(void);
void kpropagate_news_of_gained_capture_signal(void);
void kpropagate_news_of_lost_capture_signal(void);
void kpropagate_news_of_unrecoverable_error(void);
void kpropagate_news_of_new_captured_frame(void);

void kpropagate_saved_mode_params_to_disk(const std::vector<video_mode_params_s> &modeParams, const std::string &targetFilename);
void kpropagate_saved_aliases_to_disk(const std::vector<mode_alias_s> &aliases, const std::string &targetFilename);

void kpropagate_loaded_mode_params_from_disk(const std::vector<video_mode_params_s> &modeParams, const std::string &sourceFilename);
void kpropagate_loaded_aliases_from_disk(const std::vector<mode_alias_s> &aliases, const std::string &sourceFilename);

void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta);
void kpropagate_forced_capture_resolution(const resolution_s &r);

#endif
