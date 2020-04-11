/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * An interface for non-GUI code to interact with the GUI.
 *
 * In VCS, the GUI doesn't actively monitor the rest of the program, e.g. frame
 * capture, to keep itself updated. Instead, those other parts of the program are
 * expected to inform the GUI of relevant events via this interface, to which the
 * GUI is then expected to react, for instance by updating itself accordingly.
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>
#include <vector>
#include "common/types.h"

struct log_entry_s;
struct mode_alias_s;

class FilterGraphNode;

enum class filter_type_enum_e;

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

// An option about a given property about the GUI's filter graph. This might be
// related to, for instance, the styling of some aspect of the graph or the like.
struct filter_graph_option_s
{
    std::string propertyName;
    int value;

    filter_graph_option_s(const std::string propertyName, const int value)
    {
        this->propertyName = propertyName;
        this->value = value;

        return;
    }
};

// Create/destroy the output window.
void kd_acquire_output_window(void);
void kd_release_output_window(void);

// Enable/disable GUI controls for changing the size of the output window.
void kd_disable_output_size_controls(const bool areDisabled);

void kd_clear_filter_graph(void);

void kd_recalculate_filter_graph_chains(void);

void kd_set_filter_graph_source_filename(const std::string &sourceFilename);

void kd_set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions);

FilterGraphNode* kd_add_filter_graph_node(const filter_type_enum_e &filterType, const u8 *const initialParameterValues);

// The current output frame's contents have changed (e.g. a new frame has been
// received from the capture hardware). Get the current frame's data, and paint
// it into the output window.
void kd_redraw_output_window(void);

// Video recording of output frames has begun or ended. Update the GUI's state
// accordingly.
void kd_set_video_recording_is_active(const bool isActive);

void kd_update_output_window_title(void);

// The size of the output frame has changed. Poll its current size, and resize
// the output window accordingly.
void kd_update_output_window_size(void);

// Information about the current video recording has changed (e.g. recording has
// started or stopped). Poll its currents status, and update the GUI accordingly.
void kd_update_video_recording_metainfo(void);

// The current capture video parameters have changed. Poll their current values,
// and update the GUI accordingly.
void kd_update_video_mode_params(void);

// Aspects of the capture signal have changed. If the GUI has a means to display
// information about it, poll its status and update the GUI accordingly.
void kd_update_capture_signal_info(void);

// Spin the GUI's event loop, by processing any user input requests, etc.
void kd_spin_event_loop(void);

// Display a message box to the user. These should be headless, i.e. independent
// of the output window, so that they can be spawned even before the GUI has been
// set up otherwise (for instance, to display an error popup if something goes
// wrong while initializing a non-GUI part of VCS).
void kd_show_headless_info_message(const char *const title, const char *const msg);
void kd_show_headless_error_message(const char *const title, const char *const msg);
void kd_show_headless_assert_error_message(const char *const msg, const char * const filename, const uint lineNum);

// A new log entry has occurred. If the GUI has a means to display VCS's log
// entries, add the given entry to it.
bool kd_add_log_entry(const log_entry_s e);

// A new alias resolution has been introduced. If the GUI has a means to display
// this information, add the given alias to it.
void kd_add_alias(const mode_alias_s a);

// All aliases should be considered to have been removed. If the GUI has a means
// to display information about aliases, wipe it clean.
void kd_clear_aliases(void);

// Inform the GUI about changes to whether the capture hardware is receiving a
// signal.
void kd_set_capture_signal_reception_status(const bool receivingASignal);

// Capture video settings have been loaded (e.g. by the user) from the file whose
// name is given. If the GUI has a means to display this information, update it
// accordingly.
void kd_set_video_presets_filename(const std::string &filename);

// Returns true if the output window is in fullscreen mode (true fullscreen, not
// just as a window resized to fill the screen).
bool kd_is_fullscreen(void);

// Re-create the current known filter chains based on the user's selections in the
// filter node graph.
void kd_refresh_filter_chains(void);

uint kd_output_framerate(void);

/// Temporary. These keep track of the total processing latency in the VCS
/// pipeline, from when a frame is received from the capture hardware to when
/// it's been processed and drawn on screen in the output window. The values are
/// available as variables in the output overlay.
int kd_average_pipeline_latency(void);
int kd_peak_pipeline_latency(void);

#endif
