/*
 * 2019-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_RESOLUTION_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_RESOLUTION_H

#include "display/qt/widgets/DialogFragment.h"

class QPushButton;

namespace control_panel::capture
{
    namespace Ui { class InputResolution; }

    class InputResolution : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit InputResolution(QWidget *parent = 0);

        ~InputResolution(void);

        void activate_resolution_button(const uint buttonIdx);

    private:
        Ui::InputResolution *ui;

        void update_button_label(QPushButton *const button);
    };
}

#endif
