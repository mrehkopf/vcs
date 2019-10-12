#ifndef D_RECORD_DIALOG_H
#define D_RECORD_DIALOG_H

#include <QDialog>

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

signals:
    void recording_started(void);

    void recording_stopped(void);

private:
    bool apply_x264_registry_settings(void);

    Ui::RecordDialog *ui;
};

#endif
