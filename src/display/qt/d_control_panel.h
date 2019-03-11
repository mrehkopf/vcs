/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_CONTROL_PANEL_H
#define D_CONTROL_PANEL_H

#include <QDialog>
#include "../../common/types.h"

class MainWindow;
class AliasDialog;
class FilterSetsListDialog;
class AntiTearDialog;
class QTreeWidgetItem;
class VideoAndColorDialog;

struct log_entry_s;
struct resolution_s;
struct mode_alias_s;
struct input_signal_s;

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QDialog
{
    Q_OBJECT

public:
    explicit ControlPanel(MainWindow *const mainWin, QWidget *parent = 0);
    ~ControlPanel();

    void add_gui_log_entry(const log_entry_s &e);

    void update_input_signal_info(const input_signal_s &s);

    void update_output_framerate(const uint fps, const bool hasMissedFrames);

    void update_output_resolution_info(void);

    void set_input_info_as_no_signal();

    void set_input_info_as_receiving_signal();

    void adjust_output_scaling(const int dir);

    bool is_mouse_wheel_scaling_allowed();

    QString GetString_OutputResolution();

    QString GetString_InputRefreshRate();

    QString GetString_InputResolution();

    QString GetString_OutputFrameRate();

    QString GetString_OutputLatency();

    QString GetString_DroppingFrames();

    void clear_known_modes();

    void notify_of_new_alias(const mode_alias_s a);

    void clear_known_aliases();

    void set_overlay_indicator_checked(const bool checked);

    void notify_of_new_mode_settings_source_file(const QString &filename);

    void update_current_filter_set_idx(const int idx);

private slots:
    void on_checkBox_logInfo_toggled(bool);

    void on_checkBox_logDebug_toggled(bool);

    void on_checkBox_logErrors_toggled(bool);

    void parse_input_resolution_button_press(QWidget *button);

    void on_checkBox_forceOutputAspect_stateChanged(int arg1);

    void on_comboBox_frameSkip_currentIndexChanged(const QString &arg1);

    void on_checkBox_forceOutputRes_stateChanged(int arg1);

    void on_comboBox_outputUpscaleFilter_currentIndexChanged(const QString &arg1);

    void on_comboBox_outputDownscaleFilter_currentIndexChanged(const QString &arg1);

    void on_checkBox_logEnabled_stateChanged(int arg1);

    void on_spinBox_outputScale_valueChanged(int);

    void on_spinBox_outputAspectX_valueChanged(int);

    void on_spinBox_outputAspectY_valueChanged(int);

    void on_spinBox_outputResX_valueChanged(int);

    void on_spinBox_outputResY_valueChanged(int);

    void on_pushButton_inputAdjustVideoColor_clicked();

    void on_checkBox_forceOutputScale_stateChanged(int arg1);

    void on_comboBox_inputChannel_currentIndexChanged(int index);

    void on_pushButton_inputAliases_clicked();

    void on_pushButton_editOverlay_clicked();

    void on_checkBox_overlay_stateChanged(int arg1);

    void on_comboBox_bitDepth_currentIndexChanged(const QString &arg1);

    void on_checkBox_outputAntiTear_stateChanged(int arg1);

    void on_pushButton_antiTearOptions_clicked();

    void on_pushButton_configureFiltering_clicked();

    void on_checkBox_customFiltering_stateChanged(int arg1);

    void on_checkBox_forceOutputPad_stateChanged(int arg1);

    void on_spinBox_outputPadX_valueChanged(int);

    void on_spinBox_outputPadY_valueChanged(int);

private:
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    void refresh_log_list_filtering();

    void connect_input_resolution_buttons();

    void fill_hardware_info_table();

    void fill_output_scaling_filter_comboboxes();

    void fill_input_channel_combobox();

    void set_input_controls_enabled(const bool state);

    void set_output_controls_enabled(const bool state);

    void filter_log_entry(QTreeWidgetItem *const entry);

    void reset_capture_bit_depth_combobox();

    VideoAndColorDialog *videocolorDlg = nullptr;

    AliasDialog *aliasDlg = nullptr;

    FilterSetsListDialog *filterSetsDlg = nullptr;

    AntiTearDialog *antitearDlg = nullptr;

    Ui::ControlPanel *ui;
};

#endif // D_CONTROL_PANEL_H
