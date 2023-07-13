#ifndef VCS_DISPLAY_QT_DIALOGS_CAPTURE_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_CAPTURE_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

namespace Ui {
class CaptureDialog;
}

class InputChannel;
class SignalStatus;
class InputResolution;

class CaptureDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit CaptureDialog(QWidget *parent = nullptr);
    ~CaptureDialog();

    InputChannel* input_channel(void)       const { return this->inputChannel; }
    SignalStatus* signal_status(void)       const { return this->signalStatus; }
    InputResolution* input_resolution(void) const { return this->inputResolution; }

private:
    Ui::CaptureDialog *ui;

    InputChannel *inputChannel = nullptr;
    SignalStatus *signalStatus = nullptr;
    InputResolution *inputResolution = nullptr;
};

#endif
