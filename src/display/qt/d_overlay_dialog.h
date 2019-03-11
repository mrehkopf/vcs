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
class QSignalMapper;

namespace Ui {
class OverlayDialog;
}

class OverlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OverlayDialog(MainWindow *const mainWin, QWidget *parent = 0);
    ~OverlayDialog();

    bool is_overlay_enabled();

    QString overlay_html_string();

    void set_overlay_enabled(const bool state);

private slots:
    void insert_text_into_overlay(const QString &t);

    void add_image_to_overlay();

    void on_groupBox_overlay_toggled(bool);

private:
    void add_menu_item(QMenu *const menu, QSignalMapper *const sm, const QString &menuText,
                     const QString &outText);

    void make_button_menus();

    void finalize_overlay_string();

    Ui::OverlayDialog *ui;
};

#endif // D_OVERLAY_DIALOG_H
