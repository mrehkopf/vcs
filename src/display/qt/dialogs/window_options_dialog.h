#ifndef VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH
#define VCS_DISPLAY_QT_DIALOGS_WINDOW_OPTIONS_DIALOGH

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace Ui {
class WindowOptionsDialog;
}

class OutputWindow;
class WindowSize;
class WindowScaler;
class WindowRenderer;

class WindowOptionsDialog : public VCSDialogFragment
{
    Q_OBJECT

public:
    explicit WindowOptionsDialog(QWidget *parent = nullptr);
    ~WindowOptionsDialog();

    WindowSize* window_size(void) const { return this->windowSize; }

private:
    Ui::WindowOptionsDialog *ui;

    WindowSize *windowSize = nullptr;
    WindowScaler *windowScaler = nullptr;
    WindowRenderer *windowRenderer = nullptr;
};

#endif
