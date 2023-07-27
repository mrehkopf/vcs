#ifndef VCS_DISPLAY_QT_DIALOGS_CAPTURE_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_CAPTURE_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace control_panel::capture
{
    class InputChannel;
    class SignalStatus;
    class InputResolution;
}

namespace control_panel
{
    namespace Ui { class Capture; }

    class Capture : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit Capture(QWidget *parent = nullptr);
        ~Capture();

        control_panel::capture::InputChannel* input_channel(void) const { return this->inputChannel; }
        control_panel::capture::SignalStatus* signal_status(void)       const { return this->signalStatus; }
        control_panel::capture::InputResolution* input_resolution(void) const { return this->inputResolution; }

    private:
        Ui::Capture *ui;

        control_panel::capture::InputChannel *inputChannel = nullptr;
        control_panel::capture::SignalStatus *signalStatus = nullptr;
        control_panel::capture::InputResolution *inputResolution = nullptr;
    };

    #endif
}
