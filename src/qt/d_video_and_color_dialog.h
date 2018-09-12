/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_VIDEO_AND_COLOR_DIALOG_H
#define D_VIDEO_AND_COLOR_DIALOG_H

#include <QDialog>

struct resolution_s;
struct input_signal_s;
struct input_video_params_s;
struct input_color_params_s;

class QGroupBox;

namespace Ui {
class VideoAndColorDialog;
}

class VideoAndColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoAndColorDialog(QWidget *parent = 0);
    ~VideoAndColorDialog();

    void update_controls();

    void set_controls_enabled(const bool state);

    void receive_new_mode(const resolution_s r);

    void clear_known_modes();

private slots:
    void on_pushButton_saveModes_clicked();

    void on_pushButton_loadModes_clicked();

    void on_comboBox_knownModes_currentIndexChanged(int index);

    void BroadcastChanges();

private:
    input_video_params_s current_video_params();

    input_color_params_s current_color_params();

    void update_video_controls();

    void update_color_controls();

    Ui::VideoAndColorDialog *ui;
    void connect_spinboxes_to_their_sliders(QGroupBox * const parent);
};

#endif // D_VIDEO_AND_COLOR_DIALOG_H
