/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_SIGNAL_STATUS_H
#define VCS_DISPLAY_QT_DIALOGS_COMPONENTS_CAPTURE_DIALOG_SIGNAL_STATUS_H

#include "display/qt/widgets/QDialog_vcs_dialog_fragment.h"

class QMenuBar;

struct capture_video_settings_s;
struct capture_color_settings_s;
struct resolution_s;

namespace control_panel::capture
{
    namespace Ui { class SignalStatus; }

    class SignalStatus : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit SignalStatus(QWidget *parent = 0);
        ~SignalStatus(void);

        void set_controls_enabled(const bool state);

        void update_controls(void);

    private:
        void update_information_table(const bool isReceivingSignal);

        Ui::SignalStatus *ui;
    };
}

#endif
