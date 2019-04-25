/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef OVERLAY_DIALOG_H
#define OVERLAY_DIALOG_H

#include <QTextDocument>
#include <QDialog>

class QMenu;

namespace Ui {
class OverlayDialog;
}

class OverlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OverlayDialog(QWidget *parent = 0);
    ~OverlayDialog();

    QImage overlay_as_qimage(void);

    void set_overlay_max_width(const uint width);

private slots:
    void add_image_to_overlay(void);

private:
    void insert_text_into_overlay_editor(const QString &text);

    QString parsed_overlay_string(void);

    Ui::OverlayDialog *ui;

    // Used to render the overlay's HTML into an image.
    QTextDocument overlayDocument;
};

#endif
