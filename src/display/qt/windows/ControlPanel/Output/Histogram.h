#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_HISTOGRAM_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_HISTOGRAM_H

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    namespace Ui { class Histogram; }

    class Histogram : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Histogram(QWidget *parent = 0);
        ~Histogram();

    private:
        Ui::Histogram *ui;
    };
}

#endif
