/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_RESOLUTION_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_RESOLUTION_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class ResolutionDialog;
}

struct resolution_s;

class ResolutionDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit ResolutionDialog(const QString title, resolution_s *const r, QWidget *parent = 0);

    ~ResolutionDialog(void);

private:
    Ui::ResolutionDialog *ui;

    // A pointer to the resolution object on which this dialog operates.
    resolution_s *const resolution;
};

#endif
