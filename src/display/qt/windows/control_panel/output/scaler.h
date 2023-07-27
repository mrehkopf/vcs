#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace control_panel::output
{
    namespace Ui { class Scaler; }

    class Scaler : public VCSDialogFragment
    {
        Q_OBJECT

    public:
        explicit Scaler(QWidget *parent = nullptr);
        ~Scaler();

    private:
        Ui::Scaler *ui;
    };
}

#endif
