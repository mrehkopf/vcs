#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace control_panel::output
{
    namespace Ui { class Renderer; }

    class Renderer : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Renderer(QWidget *parent = nullptr);
        ~Renderer();

    private:
        Ui::Renderer *ui;
    };
}

#endif
