#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_WINDOW_OPTIONS_DIALOG_WINDOW_RENDERER_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace Ui {
class WindowRenderer;
}

class OutputWindow;

class WindowRenderer : public VCSDialogFragment
{
    Q_OBJECT

public:
    explicit WindowRenderer(QWidget *parent = nullptr);
    ~WindowRenderer();

private:
    Ui::WindowRenderer *ui;
};

#endif
