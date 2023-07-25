#ifndef VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace Ui {
class AboutDialog;
}

class AboutDialog : public VCSDialogFragment
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private:
    Ui::AboutDialog *ui;
};

#endif
