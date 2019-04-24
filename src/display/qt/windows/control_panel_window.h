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
class QTreeWidgetItem;
class AntiTearDialog;
class AliasDialog;

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

    void set_capture_info_as_no_signal();

    void set_capture_info_as_receiving_signal();

    void adjust_output_scaling(const int dir);

    bool is_mouse_wheel_scaling_allowed();

    void notify_of_new_alias(const mode_alias_s a);

    void clear_known_aliases();

    void notify_of_new_mode_settings_source_file(const QString &filename);

    void update_filter_set_idx(void);

    void open_video_adjust_dialog();

    void open_antitear_dialog();

    void open_filter_sets_dialog();

    void open_alias_dialog();

    void update_recording_metainfo();

    void toggle_overlay();

    void activate_capture_res_button(const uint buttonIdx);

    void update_video_params();

    void update_filter_sets_list();

    bool is_overlay_enabled(void);

    bool custom_program_styling_enabled();

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

    FilterSetsListDialog *filterSetsDlg = nullptr;
    VideoAndColorDialog *videocolorDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    AliasDialog *aliasDlg = nullptr;

    ControlPanelAboutWidget *aboutWidget = nullptr;
    ControlPanelRecordWidget *recordWidget = nullptr;
    ControlPanelOutputWidget *outputWidget = nullptr;
    ControlPanelInputWidget *inputWidget = nullptr;

    Ui::ControlPanel *ui;
};

#endif
