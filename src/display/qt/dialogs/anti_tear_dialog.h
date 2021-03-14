/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_ANTI_TEAR_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ANTI_TEAR_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class QMenuBar;

namespace Ui {
class AntiTearDialog;
}

class AntiTearDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit AntiTearDialog(QWidget *parent = 0);

    ~AntiTearDialog(void);

private:
    void update_visualization_options(void);

    Ui::AntiTearDialog *ui;
};

#endif
