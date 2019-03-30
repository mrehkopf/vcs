/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_VIDEO_AND_COLOR_DIALOG_H
#define D_VIDEO_AND_COLOR_DIALOG_H

#include <QDialog>

struct resolution_s;
struct capture_signal_s;
struct input_video_settings_s;
struct input_color_settings_s;

class QGroupBox;
class QMenuBar;

namespace Ui {
class VideoAndColorDialog;
}

class VideoAndColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoAndColorDialog(QWidget *parent = 0);
    ~VideoAndColorDialog();

    void notify_of_new_capture_signal(void);

    void set_controls_enabled(const bool state);

    void clear_known_modes();

    void receive_new_mode_settings_filename(const QString &filename);

    void update_controls();

private slots:
    void save_settings(void);

    void load_settings(void);

    void broadcast_settings_change();

private:
    input_video_settings_s current_video_params();

    input_color_settings_s current_color_params();

    void connect_spinboxes_to_their_sliders(QGroupBox *const parent);

    void create_menu_bar();

    void update_video_controls();

    void update_color_controls();

    void flag_unsaved_changes();

    Ui::VideoAndColorDialog *ui;

    QMenuBar *menubar = nullptr;
};

#endif // D_VIDEO_AND_COLOR_DIALOG_H
