/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef SIGNAL_DIALOG_H
#define SIGNAL_DIALOG_H

#include <QDialog>

class QMenuBar;

namespace Ui {
class SignalDialog;
}

struct capture_video_settings_s;
struct capture_color_settings_s;
struct signal_info_s;
struct resolution_s;

class SignalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignalDialog(QWidget *parent = 0);
    ~SignalDialog();

    void notify_of_new_capture_signal(void);

    void set_controls_enabled(const bool state);

    void receive_new_mode_settings_filename(const QString &filename);

    void update_controls(void);

private slots:
    void save_settings(void);

    void load_settings(void);

    void broadcast_settings(void);

private:
    void flag_unsaved_changes(void);

    void remove_unsaved_changes_flag(void);

    void update_information_table(const bool isReceivingSignal);

    Ui::SignalDialog *ui;

    QMenuBar *menubar = nullptr;
};

#endif
