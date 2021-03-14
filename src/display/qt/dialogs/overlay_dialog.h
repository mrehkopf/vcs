/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H

#include <QTextDocument>
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class QMenu;
class QMenuBar;

namespace Ui {
class OverlayDialog;
}

class OverlayDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit OverlayDialog(QWidget *parent = 0);
    ~OverlayDialog();

    QImage overlay_as_qimage(void);

    void set_overlay_max_width(const uint width);

private:
    void insert_text_into_overlay_editor(const QString &text);

    QString parsed_overlay_string(void);

    Ui::OverlayDialog *ui;

    // Used to render the overlay's HTML into an image.
    QTextDocument overlayDocument;
};

#endif
