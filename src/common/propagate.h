#ifndef PROPAGATE_H
#define PROPAGATE_H

struct resolution_s;

void kpropagate_news_of_new_capture_video_mode(void);

void kpropagate_news_of_invalid_capture_signal(void);

void kpropagate_news_of_lost_capture_signal(void);

void kpropagate_news_of_gained_capture_signal(void);

void kpropagate_news_of_new_captured_frame(void);

void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta);

void kpropagate_news_of_unrecoverable_error(void);

void kpropagate_forced_capture_resolution(const resolution_s &r);

#endif
