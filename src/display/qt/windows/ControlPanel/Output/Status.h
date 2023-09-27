/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_OUTPUT_STATUS_H
#define VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_OUTPUT_STATUS_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    namespace Ui { class Status; }

    class OutputStatus : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit OutputStatus(QWidget *parent = 0);

    private:
        Ui::Status *ui;
    };
}

#endif
