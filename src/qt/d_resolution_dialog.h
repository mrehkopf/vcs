/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_RESOLUTION_DIALOG_H
#define D_RESOLUTION_DIALOG_H

#include <QDialog>

struct resolution_s;

namespace Ui {
class ResolutionDialog;
}

class ResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResolutionDialog(const QString title, resolution_s *const r, QWidget *parent = 0);
    ~ResolutionDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::ResolutionDialog *ui;
};

#endif // D_RESOLUTION_DIALOG_H
