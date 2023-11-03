/*
 * 2023 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <array>
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
#include "Status.h"
#include "ui_Status.h"

static QTimer INFO_UPDATE_TIMER;

// For keeping a running average of the capture latency.
static unsigned LATENCY_HISTORY_HEAD = 0;
static std::array<unsigned, 140> LATENCY_HISTORY;

control_panel::output::Status::Status(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Status)
{
    ui->setupUi(this);

    this->set_name("Output status");

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information. Note that this also sets
        // the vertical order in which the table's parameters are shown.
        ui->tableWidget_propertyTable->add_property("Resolution");
        ui->tableWidget_propertyTable->add_property("Frame rate");
        ui->tableWidget_propertyTable->add_property(
            "Processing latency",
            "Time spent by VCS to process and display a captured frame"
        );
        ui->tableWidget_propertyTable->add_property("Frames dropped");

        INFO_UPDATE_TIMER.start(1000);
        connect(&INFO_UPDATE_TIMER, &QTimer::timeout, [this]
        {
            ui->tableWidget_propertyTable->modify_property("Frames dropped", QString::number(kc_dropped_frames_count()));
        });
    }

    // Listen for app events.
    {
        ev_capture_processing_latency.listen([this](const unsigned latency)
        {
            LATENCY_HISTORY[(LATENCY_HISTORY_HEAD++) % LATENCY_HISTORY.size()] = latency;
            const double avg = ((std::accumulate(LATENCY_HISTORY.begin(), LATENCY_HISTORY.end(), 0) / LATENCY_HISTORY.size()) / 1000.0);
            const double peak = (*std::max_element(LATENCY_HISTORY.begin(), LATENCY_HISTORY.end()) / 1000.0);
            ui->tableWidget_propertyTable->modify_property("Processing latency", (QString::number(avg, 'f', 1) + " ms, " + QString::number(peak, 'f', 1) + " ms peak"));
        });

        ev_new_output_image.listen([this](const image_s &image)
        {
            ui->tableWidget_propertyTable->modify_property(
                "Resolution",
                QString("%1 \u00d7 %2")
                    .arg(image.resolution.w)
                    .arg(image.resolution.h)
            );
        });

        ev_frames_per_second.listen([this](const refresh_rate_s &fps)
        {
            if (kc_has_signal())
            {
                this->ui->tableWidget_propertyTable->modify_property("Frame rate", QString("%1 FPS").arg(fps.value<double>()));
            }
        });

        ev_capture_signal_lost.listen([this]
        {
            ui->tableWidget_propertyTable->reset_property_values();
        });
    }

    return;
}

control_panel::output::Status::~Status(void)
{
    delete ui;
    ui = nullptr;

    return;
}
