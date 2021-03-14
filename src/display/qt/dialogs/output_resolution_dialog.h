#ifndef VCS_DISPLAY_QT_DIALOGS_OUTPUT_RESOLUTION_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_OUTPUT_RESOLUTION_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class OutputResolutionDialog;
}

class OutputResolutionDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit OutputResolutionDialog(QWidget *parent = 0);
    ~OutputResolutionDialog();

    void adjust_output_scaling(const int dir);

    void disable_output_size_controls(const bool areDisabled);

    void notify_of_new_capture_signal(void);

private:
    Ui::OutputResolutionDialog *ui;
};

#endif
