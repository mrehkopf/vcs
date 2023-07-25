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
#include "display/qt/subclasses/QTableWidget_property_table.h"
#include "common/vcs_event/vcs_event.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture.h"
#include "common/disk/disk.h"
#include "signal_status.h"
#include "ui_signal_status.h"

// Used to keep track of how long we've had a particular video mode set.
static QTimer INFO_UPDATE_TIMER;

SignalStatus::SignalStatus(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::SignalStatus)
{
    ui->setupUi(this);

    this->set_name("Signal info");

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information. Note that this also sets
        // the vertical order in which the table's parameters are shown.
        ui->tableWidget_propertyTable->modify_property("Resolution", "-");
        ui->tableWidget_propertyTable->modify_property("Input rate", "-");
        ui->tableWidget_propertyTable->modify_property("Output rate", "-");
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

        ev_frames_per_second.listen([this](const unsigned fps)
        {
            ui->tableWidget_propertyTable->modify_property("Output rate", QString("%1 FPS").arg(fps));
        });

        ev_new_video_mode.listen([update_info](const video_mode_s&)
        {
            update_info();
        });

        ev_new_input_channel.listen(update_info);
    }

    return;
}

SignalStatus::~SignalStatus(void)
{
    delete ui;
    ui = nullptr;

    return;
}

void SignalStatus::set_controls_enabled(const bool state)
{
    update_information_table(state);

    return;
}

void SignalStatus::update_information_table(const bool isReceivingSignal)
{
    if (isReceivingSignal)
    {
        const auto resolution = resolution_s::from_capture_device();
        const auto refreshRate = refresh_rate_s::from_capture_device().value<double>();

        ui->tableWidget_propertyTable->modify_property("Input rate", QString("%1 Hz").arg(QString::number(refreshRate, 'f', 3)));
        ui->tableWidget_propertyTable->modify_property("Resolution", QString("%1 \u00d7 %2").arg(resolution.w).arg(resolution.h));
    }
    else
    {
        ui->tableWidget_propertyTable->modify_property("Resolution", "-");
        ui->tableWidget_propertyTable->modify_property("Input rate", "-");
    }

    return;
}
