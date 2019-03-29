#ifndef PROPAGATE_H
#define PROPAGATE_H

void kpropagate_new_input_video_mode(void);

void kpropagate_invalid_input_signal(void);

void kpropagate_lost_input_signal(void);

void kpropagate_gained_input_signal(void);

void kpropagate_new_captured_frame(void);

void kpropagate_capture_alignment_adjust(const int horizontalDelta, const int verticalDelta);

void kpropagate_unrecoverable_error(void);

#endif
