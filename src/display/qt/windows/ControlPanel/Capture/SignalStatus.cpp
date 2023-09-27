/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QElapsedTimer>
#include <QFileDialog>
#include <QPushButton>
#include <QFileInfo>
#include <QToolBar>
#include <QMenuBar>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include "display/qt/widgets/PropertyTable.h"
#include "common/vcs_event/vcs_event.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/disk/disk.h"
#include "SignalStatus.h"
#include "ui_SignalStatus.h"

static QTimer INFO_UPDATE_TIMER;

control_panel::capture::SignalStatus::SignalStatus(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::SignalStatus)
{
    ui->setupUi(this);

    this->set_name("Signal info");

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information. Note that this also sets
        // the vertical order in which the table's parameters are shown.
        ui->tableWidget_propertyTable->modify_property("Resolution", "-");
        ui->tableWidget_propertyTable->modify_property("Refresh rate", "-");
        ui->tableWidget_propertyTable->modify_property("Frames dropped", "-");

        INFO_UPDATE_TIMER.start(1000);
        connect(&INFO_UPDATE_TIMER, &QTimer::timeout, [this]
        {
            ui->tableWidget_propertyTable->modify_property("Frames dropped", QString::number(kc_dropped_frames_count()));
        });
    }

    // Listen for app events.
    {
        const auto update_info = [this]
        {
            update_information_table(kc_has_signal());
        };

        ev_capture_signal_lost.listen([this]
        {
            this->set_controls_enabled(false);
        });

        ev_capture_signal_gained.listen([this]
        {
            this->set_controls_enabled(true);
        });

        ev_new_video_mode.listen([update_info](const video_mode_s&)
        {
            update_info();
        });

        ev_new_input_channel.listen(update_info);
    }

    return;
}

control_panel::capture::SignalStatus::~SignalStatus(void)
{
    delete ui;
    ui = nullptr;

    return;
}

void control_panel::capture::SignalStatus::set_controls_enabled(const bool state)
{
    update_information_table(state);

    return;
}

void control_panel::capture::SignalStatus::update_information_table(const bool isReceivingSignal)
{
    if (isReceivingSignal)
    {
        const auto resolution = resolution_s::from_capture_device();
        const auto refreshRate = refresh_rate_s::from_capture_device().value<double>();

        ui->tableWidget_propertyTable->modify_property("Refresh rate", QString("%1 Hz").arg(QString::number(refreshRate, 'f', 3)));
        ui->tableWidget_propertyTable->modify_property("Resolution", QString("%1 \u00d7 %2").arg(resolution.w).arg(resolution.h));
    }
    else
    {
        ui->tableWidget_propertyTable->modify_property("Resolution", "-");
        ui->tableWidget_propertyTable->modify_property("Rate", "-");
    }

    return;
}
