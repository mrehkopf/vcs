/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef OUTPUT_WINDOW_H
#define OUTPUT_WINDOW_H

#include <QMainWindow>
#include "display/display.h"
#include "common/globals.h"

class OutputResolutionDialog;
class InputResolutionDialog;
class VideoAndColorDialog;
class FilterGraphDialog;
class AntiTearDialog;
class OverlayDialog;
class RecordDialog;
class AliasDialog;
class AboutDialog;

struct capture_signal_s;
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

    u32 ShowMessageBox_Query(const QString title, const QString msg);

    void ShowMessageBox_Information(const QString msg);

    void ShowMessageBox_Error(const QString msg);

    void add_gui_log_entry(const log_entry_s e);

    void update_capture_signal_info(void);

    void update_output_framerate(const u32 fps, const bool missedFrames);

    void set_capture_info_as_no_signal(void);

    void set_capture_info_as_receiving_signal(void);

    void update_window_size(void);

    void update_window_title(void);

    void signal_new_known_alias(const mode_alias_s a);

    void clear_known_aliases(void);

    void open_overlay_dialog(void);

    void measure_framerate(void);

    void signal_new_mode_settings_source_file(const std::string &filename);

    void set_opengl_enabled(const bool enabled);

    void refresh(void);

    QImage overlay_image(void);

    void update_recording_metainfo(void);

    void update_video_mode_params(void);

    uint output_framerate(void);

    void clear_filter_graph(void);

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

    bool is_mouse_wheel_scaling_allowed(void);

    void open_input_resolution_dialog(void);

    void disable_output_size_controls(const bool areDisabled);

    void set_recording_is_active(const bool isActive);

private slots:
    void toggle_window_border(void);

private:
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);

    void set_keyboard_shortcuts();

    bool load_font(const QString &filename);

    Ui::MainWindow *ui = nullptr;

    OutputResolutionDialog *outputResolutionDlg = nullptr;
    InputResolutionDialog *inputResolutionDlg = nullptr;
    VideoAndColorDialog *videoDlg = nullptr;
    FilterGraphDialog *filterGraphDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    OverlayDialog *overlayDlg = nullptr;
    RecordDialog *recordDlg = nullptr;
    AliasDialog *aliasDlg = nullptr;
    AboutDialog *aboutDlg = nullptr;

    // Set to true when the user has selected to close the
    // main window.
    bool userExiting = false;

    // Set to true if the program failed to initialize the capture card.
    bool captureInitFail = false;
};

#endif
