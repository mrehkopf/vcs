#ifndef D_RECORD_DIALOG_H
#define D_RECORD_DIALOG_H

#include <QDialog>

class QMenuBar;

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

private:
    bool apply_x264_registry_settings(void);

    Ui::RecordDialog *ui;

    // Whether recording is enabled (on).
    bool isEnabled = false;

    QMenuBar *menubar = nullptr;
};

#endif
