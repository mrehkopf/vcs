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
class QMenuBar;

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

    void set_overlay_enabled(const bool enabled);

    bool is_overlay_enabled(void);

signals:
    // Emitted when the overlay's enabled status is toggled.
    void overlay_enabled(void);
    void overlay_disabled(void);

private:
    void insert_text_into_overlay_editor(const QString &text);

    QString parsed_overlay_string(void);

    Ui::OverlayDialog *ui;

    // Whether the overlay is enabled.
    bool isEnabled = false;

    // Used to render the overlay's HTML into an image.
    QTextDocument overlayDocument;

    QMenuBar *menubar = nullptr;
};

#endif
