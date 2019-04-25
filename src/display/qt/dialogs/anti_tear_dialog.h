/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef ANTI_TEAR_DIALOG_H
#define ANTI_TEAR_DIALOG_H

#include <QDialog>

namespace Ui {
class AntiTearDialog;
}

class AntiTearDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AntiTearDialog(QWidget *parent = 0);

    ~AntiTearDialog();

private:
    Ui::AntiTearDialog *ui;
};

#endif
