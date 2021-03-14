#ifndef VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ABOUT_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class AboutDialog;
}

class AboutDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

    void notify_of_new_program_version(void);

private:
    Ui::AboutDialog *ui;
};

#endif

