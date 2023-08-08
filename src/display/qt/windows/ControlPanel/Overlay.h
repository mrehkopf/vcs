/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_OVERLAY_DIALOG_H

#include <QTextDocument>
#include "capture/capture.h"
#include "display/qt/widgets/DialogFragment.h"

namespace control_panel
{
    namespace Ui { class Overlay; }

    class Overlay : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Overlay(QWidget *parent = 0);
        ~Overlay();

        QImage rendered(void);

        void set_overlay_max_width(const uint width);

    private:
        Ui::Overlay *ui;

        // Used to render the overlay's HTML into an image.
        QTextDocument overlayDocument;
    };
}

#endif
