/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef OUTPUT_WINDOW_H
#define OUTPUT_WINDOW_H

#include <QMainWindow>
#include "display/qt/utility.h"
#include "display/display.h"
#include "common/globals.h"

class OutputResolutionDialog;
class InputResolutionDialog;
class FilterGraphDialog;
class AntiTearDialog;
class OverlayDialog;
class RecordDialog;
class SignalDialog;
class AliasDialog;
class AboutDialog;

struct signal_info_s;
struct mode_alias_s;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // Gets user input in the GUI, etc.
    void update_gui_state(void);

    // Returns true if the window has a border.
    bool window_has_border(void);

    void add_gui_log_entry(const log_entry_s e);

    void update_capture_signal_info(void);

    void update_output_framerate(const u32 fps, const bool missedFrames);

    void set_capture_info_as_no_signal(void);

    void set_capture_info_as_receiving_signal(void);

    void update_window_size(void);

    // Updates the window title with the current status of capture and that of the program
    // in general.
    void update_window_title(void);

    void signal_new_known_alias(const mode_alias_s a);

    void clear_known_aliases(void);

    void open_overlay_dialog(void);

    // Call this once per frame to allow the output window to estimate the
    // current frame rate.
    void measure_framerate(void);

    void signal_new_mode_settings_source_file(const std::string &filename);

    void set_opengl_enabled(const bool enabled);

    void refresh(void);

    // Returns the current overlay as a QImage, or a null QImage if the overlay
    // should not be shown at this time.
    QImage overlay_image(void);

    void update_recording_metainfo(void);

    void update_video_mode_params(void);

    uint output_framerate(void);

    void clear_filter_graph(void);

    void recalculate_filter_graph_chains(void);

    FilterGraphNode* add_filter_graph_node(const filter_type_enum_e &filterType, const u8 * const initialParameterValues);

    void set_filter_graph_source_filename(const std::string &sourceFilename);

    void set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions);

    void open_filter_graph_dialog(void);

    void open_antitear_dialog(void);

    void open_video_dialog(void);

    void open_alias_dialog(void);

    void open_about_dialog(void);

    void open_record_dialog(void);

    void open_output_resolution_dialog(void);

    // Returns true if the user is allowed to scale the output resolution by using the
    // mouse wheel over the capture window.
    bool is_mouse_wheel_scaling_allowed(void);

    void open_input_resolution_dialog(void);

    void disable_output_size_controls(const bool areDisabled);

    void set_recording_is_active(const bool isActive);

signals:
    void entered_fullscreen(void);
    void left_fullscreen(void);

    // Emitted when the window border is toggled.
    void border_hidden(void);
    void border_revealed(void);

private slots:
    void toggle_window_border(void);

private:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void wheelEvent(QWheelEvent *event);

    void set_keyboard_shortcuts(void);

    // Load the font pointed to by the given filename, and make it available to the program.
    bool load_font(const QString &filename);

    Ui::MainWindow *ui = nullptr;

    OutputResolutionDialog *outputResolutionDlg = nullptr;
    InputResolutionDialog *inputResolutionDlg = nullptr;
    SignalDialog *signalDlg = nullptr;
    FilterGraphDialog *filterGraphDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    OverlayDialog *overlayDlg = nullptr;
    RecordDialog *recordDlg = nullptr;
    AliasDialog *aliasDlg = nullptr;
    AboutDialog *aboutDlg = nullptr;

    // A master list of all the dialogs that this window spawns. Will be filled in
    // when we create the dialog instances.
    QVector<QDialog*> dialogs;

    mouse_activity_monitor_c mouseActivityMonitor;

    // Set to true when the user has selected to close the
    // main window.
    bool userExiting = false;

    // Set to true if the program failed to initialize the capture card.
    bool captureInitFail = false;

    // A user-definable custom title for the output window.
    QString windowTitleOverride = "";

    const Qt::WindowFlags defaultWindowFlags = (Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
};

#endif
