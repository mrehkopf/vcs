#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_SIZE_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    namespace Ui { class Size; }

    class Size : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Size(QWidget *parent = 0);
        ~Size();

        void adjust_output_scaling(const int dir);

        void disable_output_size_controls(const bool areDisabled);

        void notify_of_new_capture_signal(void);

    private:
        Ui::Size *ui;
    };
}

#endif
