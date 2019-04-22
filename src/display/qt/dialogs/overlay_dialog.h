/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_OVERLAY_DIALOG_H
#define D_OVERLAY_DIALOG_H

#include <QDialog>

class QMenu;
class MainWindow;

namespace Ui {
class OverlayDialog;
}

class OverlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OverlayDialog(MainWindow *const mainWin, QWidget *parent = 0);
    ~OverlayDialog();

    QImage overlay_as_qimage();

    void set_overlay_max_width(const uint width);

    void update_stylesheet(const QString &stylesheet);

private slots:
    void insert_text_into_overlay(const QString &t);

    void add_image_to_overlay();

private:
    void add_menu_item(QMenu *const menu, const QString &menuText,
                       const QString &outText);

    void make_button_menus();

    QString parsed_overlay_string();

    Ui::OverlayDialog *ui;
};

#endif // D_OVERLAY_DIALOG_H
