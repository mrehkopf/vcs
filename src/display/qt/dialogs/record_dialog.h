#ifndef VCS_DISPLAY_QT_DIALOGS_RECORD_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_RECORD_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class QMenuBar;
class QTimer;

namespace Ui {
class RecordDialog;
}

class RecordDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit RecordDialog(QDialog *parent = 0);
    ~RecordDialog();

    void update_recording_metainfo(void);

    void set_recording_controls_enabled(const bool areEnabled);

signals:
    // Emitted when the recording's enabled status is toggled.
    void recording_could_not_be_disabled(void);
    void recording_could_not_be_enabled(void);

private:
    bool apply_x264_registry_settings(void);

    Ui::RecordDialog *ui;

    // While VCS is recording, this timer will provide an interval for updating
    // the GUI's recording info (duration of recording, size of the video file,
    // etc.).
    QTimer *timerUpdateRecordingInfo = nullptr;
};

#endif
