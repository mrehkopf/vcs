/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H

#include <QTextDocument>
#include "capture/capture.h"
#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

class QMenu;
class QMenuBar;

namespace Ui {
class OverlayDialog;
}

class OverlayDialog : public VCSDialogFragment
{
    Q_OBJECT

public:
    explicit OverlayDialog(QWidget *parent = 0);
    ~OverlayDialog();

    QImage rendered(void);

    void set_overlay_max_width(const uint width);

private:
    Ui::OverlayDialog *ui;

    // Used to render the overlay's HTML into an image.
    QTextDocument overlayDocument;
};

#endif
