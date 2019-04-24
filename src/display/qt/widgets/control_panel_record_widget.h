#ifndef CONTROL_PANEL_RECORD_WIDGET_H
#define CONTROL_PANEL_RECORD_WIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelRecordWidget;
}

class ControlPanelRecordWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelRecordWidget(QWidget *parent = 0);
    ~ControlPanelRecordWidget();

    void update_recording_metainfo();

private slots:
    void on_pushButton_recordingStart_clicked();

    void on_pushButton_recordingStop_clicked();

    void on_pushButton_recordingSelectFilename_clicked();

signals:
    // Ask for any controls by which the user can adjust the capture output
    // size from its current settings to be enabled/disabled.
    void set_output_size_controls_enabled(const bool state);

    void update_output_window_title(void);

    void update_output_window_size(void);

private:
    Ui::ControlPanelRecordWidget *ui;

    bool apply_x264_registry_settings();
};

#endif // CONTROL_PANEL_RECORD_WIDGET_H
