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

    void update_recording_metainfo(void);

    void restore_persistent_settings(void);

signals:
    void recording_started(void);

    void recording_stopped(void);

private:
    bool apply_x264_registry_settings(void);

    Ui::ControlPanelRecordWidget *ui;
};

#endif
