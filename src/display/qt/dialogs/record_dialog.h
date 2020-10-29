#ifndef D_RECORD_DIALOG_H
#define D_RECORD_DIALOG_H

#include <QDialog>

class QMenuBar;
class QTimer;

namespace Ui {
class RecordDialog;
}

class RecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordDialog(QDialog *parent = 0);
    ~RecordDialog();

    void update_recording_metainfo(void);

    void set_recording_controls_enabled(const bool areEnabled);

    void toggle_recording(void);

    void set_recording_enabled(const bool enabled);

    bool is_recording_enabled(void);

signals:
    // Emitted when the recording's enabled status is toggled.
    void recording_enabled(void);
    void recording_disabled(void);
    void recording_could_not_be_disabled(void);
    void recording_could_not_be_enabled(void);

private:
    bool apply_x264_registry_settings(void);

    Ui::RecordDialog *ui;

    // Whether recording is enabled (on).
    bool isEnabled = false;

    QMenuBar *menubar = nullptr;

    // While VCS is recording, this timer will provide an interval for updating
    // the GUI's recording info (duration of recording, size of the video file,
    // etc.).
    QTimer *timerUpdateRecordingInfo = nullptr;
};

#endif
