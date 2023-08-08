#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SCALER_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    namespace Ui { class Scaler; }

    class Scaler : public DialogFragment
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
