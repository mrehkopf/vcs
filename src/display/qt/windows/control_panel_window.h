/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef CONTROL_PANEL_WINDOW_H
#define CONTROL_PANEL_WINDOW_H

#include <QDialog>
#include "common/types.h"

class ControlPanelRecordWidget;
class ControlPanelOutputWidget;
class ControlPanelAboutWidget;
class ControlPanelInputWidget;
class FilterSetsListDialog;
class VideoAndColorDialog;
class FilterGraphDialog;
class FilterGraphNode;
class QTreeWidgetItem;
class AntiTearDialog;
class AliasDialog;

enum class filter_type_enum_e;

struct filter_graph_option_s;
struct capture_signal_s;
struct resolution_s;
struct mode_alias_s;
struct log_entry_s;

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QDialog
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = 0);
    ~ControlPanel();

    void add_gui_log_entry(const log_entry_s &e);

    void update_capture_signal_info(void);

    void update_output_framerate(const uint fps, const bool hasMissedFrames);

    void update_output_resolution_info(void);

    void set_capture_info_as_no_signal(void);

    void set_capture_info_as_receiving_signal(void);

    void adjust_output_scaling(const int dir);

    bool is_mouse_wheel_scaling_allowed(void);

    void notify_of_new_alias(const mode_alias_s a);

    void clear_known_aliases(void);

    void notify_of_new_mode_settings_source_file(const QString &filename);

    void open_video_adjust_dialog(void);

    void open_antitear_dialog(void);

    void open_filter_graph_dialog(void);

    void open_alias_dialog(void);

    void update_recording_metainfo(void);

    void toggle_overlay(void);

    void activate_capture_res_button(const uint buttonIdx);

    void update_video_mode_params(void);

    bool is_overlay_enabled(void);

    bool custom_program_styling_enabled(void);

    void restore_persistent_settings(void);

    void clear_filter_graph(void);

    FilterGraphNode* add_filter_graph_node(const filter_type_enum_e &filterType, const u8 * const initialParameterValues);

    void set_filter_graph_source_filename(const std::string &sourceFilename);

    void set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions);

signals:
    void new_programwide_style_file(const QString &filename);

    // Ask the output window to open the overlay dialog.
    void open_overlay_dialog(void);

    void update_output_window_title(void);

    void update_output_window_size(void);

    // Signal that the user has activated the renderer of the given name.
    void set_renderer(const QString &rendererName);

private:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *);

    void update_tab_widths();

    VideoAndColorDialog *videocolorDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    FilterGraphDialog *filterGraphDlg = nullptr;
    AliasDialog *aliasDlg = nullptr;

    ControlPanelAboutWidget *aboutWidget = nullptr;
    ControlPanelRecordWidget *recordWidget = nullptr;
    ControlPanelOutputWidget *outputWidget = nullptr;
    ControlPanelInputWidget *inputWidget = nullptr;

    Ui::ControlPanel *ui;
};

#endif
