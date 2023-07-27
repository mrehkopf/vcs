/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_CHANNEL_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_INPUT_CHANNEL_H

#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"

namespace control_panel::capture
{
    namespace Ui { class InputChannel; }

    class InputChannel : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit InputChannel(QWidget *parent = nullptr);
        ~InputChannel();

    private:
        Ui::InputChannel *ui;
    };
}

#endif
