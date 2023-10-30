/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_CAPTURE_RATE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_CAPTURE_RATE_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::capture
{
    namespace Ui { class CaptureRate; }

    class CaptureRate : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit CaptureRate(QWidget *parent = nullptr);
        ~CaptureRate();

    private:
        Ui::CaptureRate *ui;
    };
}

#endif
