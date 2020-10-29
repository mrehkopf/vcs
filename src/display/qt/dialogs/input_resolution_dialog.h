#ifndef VCS_DISPLAY_QT_DIALOGS_INPUT_RESOLUTION_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_INPUT_RESOLUTION_DIALOG_H

#include <QDialog>

namespace Ui {
class InputResolutionDialog;
}

class InputResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputResolutionDialog(QWidget *parent = 0);
    ~InputResolutionDialog();

    void activate_capture_res_button(const uint buttonIdx);

private:
    void parse_capture_resolution_button_press(QWidget *button);

    Ui::InputResolutionDialog *ui;
};

#endif
