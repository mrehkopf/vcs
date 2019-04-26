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

    void set_capture_info_as_no_signal(void);

    void set_capture_info_as_receiving_signal(void);

    void update_capture_signal_info(void);

    void activate_capture_res_button(const uint buttonIdx);

    void restore_persistent_settings(void);


signals:
    // Ask the control panel to open the video adjustment dialog.
    void open_video_adjust_dialog(void);

    // Ask the control panel to open the alias dialog.
    void open_alias_dialog(void);

private:
    void set_input_controls_enabled(const bool state);

    void parse_capture_resolution_button_press(QWidget *button);

    void reset_color_depth_combobox_selection(void);

    Ui::ControlPanelInputWidget *ui;
};

#endif
