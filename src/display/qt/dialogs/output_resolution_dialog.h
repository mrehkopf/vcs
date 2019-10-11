#ifndef OUTPUT_RESOLUTION_DIALOG_H
#define OUTPUT_RESOLUTION_DIALOG_H

#include <QDialog>

namespace Ui {
class OutputResolutionDialog;
}

class OutputResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OutputResolutionDialog(QWidget *parent = 0);
    ~OutputResolutionDialog();

    void adjust_output_scaling(const int dir);

private:
    Ui::OutputResolutionDialog *ui;
};

#endif // OUTPUT_RESOLUTION_DIALOG_H
