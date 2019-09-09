#ifndef FILTERS_DIALOG_H
#define FILTERS_DIALOG_H

#include <QDialog>

class QMenuBar;

namespace Ui {
class FiltersDialog;
}

class FiltersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FiltersDialog(QWidget *parent = 0);
    ~FiltersDialog();

private:
    Ui::FiltersDialog *ui;

    QMenuBar *menubar = nullptr;
};

#endif
