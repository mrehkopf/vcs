#ifndef VCS_DISPLAY_QT_DIALOGS_INPUT_RESOLUTION_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_INPUT_RESOLUTION_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class InputResolutionDialog;
}

class InputResolutionDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit InputResolutionDialog(QWidget *parent = 0);

    ~InputResolutionDialog(void);

    void activate_resolution_button(const uint buttonIdx);

private:
    void update_button_text(QPushButton *const button);

    Ui::InputResolutionDialog *ui;
};

#endif
