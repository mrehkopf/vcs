/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The display interface exposes a GUI to VCS.
 * 
 * The VCS GUI is responsible for providing the user with real-time means to
 * enter information (e.g. configuring capture parameters), for directing such
 * input to VCS, and for displaying captured frames and other information about
 * VCS's run-time state to the user.
 * 
 * The GUI doesn't actively poll VCS for new data. Instead, VCS uses this
 * display interface to inform the GUI when relevant data are changed (or when
 * it otherwise wants the GUI to behave in a particular way). For example, when
 * there's a new frame to be displayed, VCS will notify the GUI by calling
 * kd_redraw_output_window(). It's then the GUI's job to request the latest
 * processed frame's data from VCS and paint it onto an output surface (a
 * window, a texture, a file on disk, or something else, which is up to the
 * GUI's discretion).
 * 
 * Since the display interface decouples the GUI's implementation from the rest
 * of VCS, you can replace the GUI with minimal modification of the rest of VCS.
 * 
 * VCS's default GUI uses Qt, and its implementation of the display interface
 * can be found in @ref src/display/qt/d_main.cpp.
 * 
 * @warning
 * As of VCS 2.0.0, this interface is undergoing refactoring. Some of its
 * functions may become renamed or removed in future versions.
 */

#ifndef VCS_DISPLAY_DISPLAY_H
#define VCS_DISPLAY_DISPLAY_H

#include <string>
#include <vector>
#include "common/types.h"

struct log_entry_s;
struct mode_alias_s;
class FilterGraphNode;
enum class filter_type_enum_e;

/*!
 * @brief
 * A container struct to hold the width, height, and bits per pixel of a
 * resolution.
 */
struct resolution_s
{
    unsigned long w, h, bpp;

    bool operator==(const resolution_s &other) const
    {
        return bool((this->w == other.w) &&
                    (this->h == other.h) &&
                    (this->bpp == other.bpp));
    }

    bool operator!=(const resolution_s &other) const
    {
        return !(*this == other);
    }
};

/*!
 * @brief
 * Maps a value to a named filter graph property.
 */
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

/*!
 * Asks the GUI to create and open the output window. The output window is a
 * surface on which VCS's output frames are to be displayed by the GUI.
 * 
 * If the GUI makes use of child windows and/or dialogs, this may be a good
 * time to create them, as well.
 * 
 * It's expected that VCS will only call this function once, at program launch.
 * 
 * @see
 * kd_release_output_window(), kd_spin_event_loop()
 */
void kd_acquire_output_window(void);

/*!
 * Asks the GUI to close and destroy the output window; as well as any child
 * windows and/or dialogs.
 * 
 * It's expected that VCS will only call this function once, when the program
 * is about to exit.
 * 
 * @see
 * kd_acquire_output_window()
 */
void kd_release_output_window(void);

/*!
 * Asks the GUI to enable or disable any user-facing controls for adjusting
 * the resolution of output frames.
 * 
 * There are certain situations during which VCS requires the output size to be
 * locked; for instance, while recording captured frames to video. The GUI
 * should fully comply with this and, when disabled, visually indicate to the
 * user that such controls are locked for the time being.
 * 
 * The GUI remains free to allow the user to adjust the output size if doing so
 * has no effect on VCS's output size - that is, if this only involves the GUI
 * modifying its local data, e.g. by deeply copying a frame's data and only
 * scaling the copy.
 */
void kd_disable_output_size_controls(const bool areDisabled);

/*!
 * Instructs the GUI to erase all information (nodes, connections, etc.) from
 * its filter graph.
 * 
 * @see
 * kd_add_filter_graph_node()
 */
void kd_clear_filter_graph(void);

/*!
 * @warning
 * This function is currently unused and should be ignored until further
 * notice. It may be renamed or removed in a future version of VCS.
 */
void kd_refresh_filter_chains(void);

// Visit each node in the graph and while doing so, group together such chains of
// filters that run from an input gate through one or more filters into an output
// gate. The chains will then be submitted to the filter handler for use in applying
// the filters to captured frames.
/*!
 * Asks the GUI to inform the filter interface, @ref src/filter/filter.h, of
 * all filter chains currently configured in the GUI.
 * 
 * This assumes that the GUI provides a dialog of some sort in which the user
 * can create and modify filter chains. When this function is called, the GUI
 * is expected to enumerate those filter chains into a format consumed by the
 * filter interface and then to submit them to the filter interface.
 * 
 * In pseudocode, the GUI would do something like the following:
 * 
 * @code
 * kf_remove_all_filter_chains();
 * 
 * for (chain: guiFilterChains)
 * {
 *     const standardFilterChain = ...convert chain into standard format...
 *     kf_add_filter_chain(standardFilterChain);
 * }
 * @endcode
 */
void kd_recalculate_filter_graph_chains(void);

/*!
 * Tells the GUI to create and add into its filter graph a new node with the
 * given properties.
 * 
 * The GUI filter graph is expected to be a dialog of some sort - e.g. a visual
 * node graph - in which the user can create and modify filter chains.
 * 
 * @p initialParameterValues is an array of bytes that defines the parameters
 * of the node's filter instance (c.f. the @ref filter_c class of the filter
 * interface, @ref src/filter/filter.h). Assuming VCS's default GUI, you can
 * find which parameters are included in this array for each filter type by
 * inspecting @ref src/display/qt/widgets/filter_widgets.cpp.
 * 
 * Returns a pointer to the created node; or @a nullptr if no node was created.
 */
FilterGraphNode* kd_add_filter_graph_node(const filter_type_enum_e &filterType, const u8 *const initialParameterValues);

/*!
 * Asks the GUI to repaint its output window (e.g. because there's a new output
 * frame to be displayed).
 * 
 * As part of its repainting, the GUI is expected to fetch the latest output
 * frame's data from the scaler interface and paint it onto the output window.
 * 
 * The following sample Qt 5 GUI code creates a QImage out of the current
 * output frame's data, then paints the image onto its output window
 * (error-checking omitted):
 * 
 * @code
 * resolution_s r = ks_output_resolution();
 * u8 *pixels = ks_scaler_output_as_raw_ptr();
 * QImage frame = QImage(pixels, r.w, r.h, QImage::Format_RGB32);
 * QPainter(this).drawImage(0, 0, frame);
 * @endcode
 */
void kd_redraw_output_window(void);

/*!
 * Asks the GUI to load video presets from the given file.
 *
 * The GUI is expected to inform VCS's video presets subsystem of the new
 * presets.
 */
void kd_load_video_presets(const std::string &filename);

/*!
 * Asks the GUI to load a filter graph from the given file.
 *
 * The GUI is expected to inform VCS's filter subsystem of the new graph.
 */
void kd_load_filter_graph(const std::string &filename);

/*!
 * Asks the GUI to load alias resolutions from the given file.
 *
 * The GUI is expected to inform VCS's alias subsystem of the new aliases.
 */
void kd_load_aliases(const std::string &filename);

/*!
 * Asks the GUI to refresh the output window's title bar text.
 * 
 * VCS assumes that the output window's title bar displays certain information
 * about VCS's state - e.g. the current capture resolution. VCS will thus call
 * this function to let the GUI know that the relevant state has changed.
 * 
 * If your custom GUI implementation displays different state variables than
 * VCS's default GUI does, you may need to do custom polling of the relevant
 * state in order to be aware of changes to it.
 * 
 * @see
 * kd_update_output_window_size()
 */
void kd_update_output_window_title(void);

/*!
 * Lets the GUI know that the size of output frames has changed. The GUI should
 * update the size of its output window accordingly.
 * 
 * VCS expects the size of the output window to match the size of output
 * frames; although the GUI may choose not to honor this.
 * 
 * The current size of output frames can be obtained via the scaler interface,
 * @ref src/scaler/scaler.h, by calling ks_output_resolution().
 * 
 * The following sample Qt 5 code sizes the output window to match the size of
 * VCS's output frames (@a this is the output window's instance):
 * 
 * @code
 * resolution_s r = ks_output_resolution();
 * this->setFixedSize(r.w, r.h);
 * @endcode
 * 
 * @see
 * kd_update_output_window_title()
 */
void kd_update_output_window_size(void);

/*!
 * Asks the GUI to execute one spin of its event loop.
 * 
 * Spinning the event loop would involve e.g. repainting the output window,
 * processing any user input, etc.
 * 
 * The following sample Qt 5 code executes one spin of the event loop:
 * 
 * @code
 * QCoreApplication::sendPostedEvents();
 * QCoreApplication::processEvents();
 * @endcode
 * 
 * VCS will generally call this function at the capture's refresh rate, e.g. 70
 * times per second when capturing VGA mode 13h.
 * 
 * @note
 * If the GUI wants to match the capture's refresh rate, it's important that it
 * only repaints its output window when VCS calls this function.
 */
void kd_spin_event_loop(void);

/*!
 * Asks the GUI to display an info message to the user.
 * 
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 * 
 * The following sample Qt 5 code creates a conforming message box:
 * 
 * @code
 * QMessageBox mb;
 * 
 * mb.setWindowTitle(strlen(title)? title : "VCS has this to say");
 * mb.setText(msg);
 * mb.setStandardButtons(QMessageBox::Ok);
 * mb.setIcon(QMessageBox::Information);
 * mb.setDefaultButton(QMessageBox::Ok);
 *
 * mb.exec();
 * @endcode
 * 
 * @see
 * kd_show_headless_error_message(), kd_show_headless_assert_error_message()
 */
void kd_show_headless_info_message(const char *const title, const char *const msg);

/*!
 * Asks the GUI to display an error message to the user.
 * 
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 * 
 * @see
 * kd_show_headless_info_message(), kd_show_headless_assert_error_message()
 */
void kd_show_headless_error_message(const char *const title, const char *const msg);

/*!
 * Asks the GUI to display an assertion error message to the user.
 * 
 * The message should be deliverable with a headless GUI, i.e. requiring
 * minimal prior GUI initialization.
 * 
 * @see
 * kd_show_headless_info_message(), kd_show_headless_error_message()
 */
void kd_show_headless_assert_error_message(const char *const msg, const char * const filename, const uint lineNum);

/*!
 * Lets the GUI know that VCS has created a new log entry, and provides a copy
 * of it as a parameter.
 * 
 * If the GUI has a view to VCS's log entries, it should update that view
 * accordingly.
 */
bool kd_add_log_entry(const log_entry_s e);

/*!
 * Lets the GUI know that VCS has created a new alias resolution, and provides
 * a copy of it as a parameter.
 * 
 * If the GUI has a view to VCS's aliases, it should update that view
 * accordingly.
 */
void kd_add_alias(const mode_alias_s a);

/*!
 * Instructs the GUI to clear its view to VCS's alias resolutions (e.g. because
 * the aliases have been reset).
 * 
 * @see
 * kd_add_alias()
 */
void kd_clear_aliases(void);

/*!
 * Returns true if the output window is in fullscreen mode; false
 * otherwise.
 */
bool kd_is_fullscreen(void);

#endif
