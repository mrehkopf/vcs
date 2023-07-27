#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_RESOLUTION_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_RESOLUTION_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

class QPushButton;

namespace control_panel::capture
{
    namespace Ui { class InputResolution; }

    class InputResolution : public VCSDialogFragment
    {
        Q_OBJECT

    public:
        explicit InputResolution(QWidget *parent = 0);

        ~InputResolution(void);

        void activate_resolution_button(const uint buttonIdx);

    private:
        void update_button_text(QPushButton *const button);

        Ui::InputResolution *ui;
    };
}

#endif
