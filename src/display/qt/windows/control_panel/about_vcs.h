#ifndef VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel
{
    namespace Ui { class AboutVCS; }

    class AboutVCS : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit AboutVCS(QWidget *parent = 0);
        ~AboutVCS();

    private:
        Ui::AboutVCS *ui;
    };
}

#endif
