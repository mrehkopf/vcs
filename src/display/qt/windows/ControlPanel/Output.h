#ifndef VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH
#define VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH

#include "display/qt/widgets/DialogFragment.h"

namespace control_panel::output
{
    class Size;
    class Scaler;
    class Renderer;
    class Histogram;
}

namespace control_panel
{
    namespace Ui { class Output; }

    class Output : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Output(QWidget *parent = nullptr);
        ~Output();

        control_panel::output::Size* size(void) const { return this->sizeDialog; }

    private:
        Ui::Output *ui;

        control_panel::output::Size *sizeDialog = nullptr;
        control_panel::output::Scaler *scalerDialog = nullptr;
        control_panel::output::Renderer *rendererDialog = nullptr;
        control_panel::output::Histogram *histogramDialog = nullptr;
    };
}

#endif
