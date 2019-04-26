#ifndef CONTROL_PANEL_INPUT_WIDGET_H
#define CONTROL_PANEL_INPUT_WIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelInputWidget;
}

class ControlPanelInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelInputWidget(QWidget *parent = 0);
    ~ControlPanelInputWidget();

    void set_capture_info_as_no_signal();

    void set_capture_info_as_receiving_signal();

    void update_capture_signal_info();

    void activate_capture_res_button(const uint buttonIdx);

    void restore_persistent_settings();

private slots:
    void set_input_controls_enabled(const bool state);

    void on_comboBox_bitDepth_currentIndexChanged(const QString &bitDepthString);

    void on_comboBox_inputChannel_currentIndexChanged(int index);

    void on_comboBox_frameSkip_currentIndexChanged(const QString &skipString);

    void on_pushButton_inputAdjustVideoColor_clicked(void);

    void on_pushButton_inputAliases_clicked(void);

signals:
    // Ask the control panel to open the video adjustment dialog.
    void open_video_adjust_dialog();

    // Ask the control panel to open the alias dialog.
    void open_alias_dialog();

private:
    void parse_capture_resolution_button_press(QWidget *button);

    void reset_color_depth_combobox_selection();

    Ui::ControlPanelInputWidget *ui;
};

#endif
