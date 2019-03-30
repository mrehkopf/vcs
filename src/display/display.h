/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "../common/types.h"

class QString;

struct log_entry_s;
struct resolution_s;
struct mode_alias_s;
struct input_signal_s;

struct resolution_s
{
    unsigned long w, h, bpp;

    bool operator==(const resolution_s &other)
    {
        return bool((this->w == other.w) &&
                    (this->h == other.h) &&
                    (this->bpp == other.bpp));
    }

    bool operator!=(const resolution_s &other)
    {
        return !(*this == other);
    }
};

void kd_acquire_display(void);

void kd_update_display_size(void);

void kd_update_gui_recording_metainfo(void);

void kd_release_display(void);

void kd_process_ui_events(void);

void kd_update_display(void);

void kd_update_gui_output_framerate_info(const u32 fps, const bool missedFrames);

bool kd_add_log_entry(const log_entry_s e);

int kd_peak_system_latency(void);

int kd_average_system_latency(void);

void kd_update_gui_video_params(void);

bool kd_is_fullscreen(void);

void kd_mark_gui_input_no_signal(const bool state);

void kd_signal_new_known_mode(const resolution_s r);

void kd_signal_new_mode_settings_source_file(const QString &filename);

void kd_signal_new_known_alias(const mode_alias_s a);

void kd_clear_known_modes(void);

void kd_clear_known_aliases(void);

void kd_update_gui_input_signal_info(void);

void kd_update_gui_filter_set_idx(const int idx);

void kd_show_headless_info_message(const char *const title, const char *const msg);

void kd_show_headless_error_message(const char *const title, const char *const msg);

void kd_show_headless_assert_error_message(const char *const msg);

#endif
