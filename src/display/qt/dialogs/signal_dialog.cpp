/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS video & color dialog
 *
 * A GUI dialog for allowing the user to adjust the capture card's video and color
 * parameters.
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
#include "display/qt/dialogs/signal_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/video_parameters.h"
#include "capture/capture.h"
#include "common/disk/disk.h"
#include "ui_signal_dialog.h"

/*
 * TODOS:
 *
 * - the dialog should communicate more clearly which mode the current GUI controls
 *   are editing, and which modes are available or known about.
 *
 */

static const QString BASE_WINDOW_TITLE = "VCS - Signal Info";

// Used to keep track of how long we've had a particular video mode set.
static QElapsedTimer VIDEO_MODE_UPTIME;
static QTimer VIDEO_MODE_UPTIME_UPDATE;

SignalDialog::SignalDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SignalDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(BASE_WINDOW_TITLE);

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Set the GUI controls to their proper initial values.
    {
        // Initialize the table of information. Note that this also sets
        // the vertical order in which the table's parameters are shown.
        {
            ui->tableWidget_propertyTable->modify_property("Input channel", "No signal");
            ui->tableWidget_propertyTable->modify_property("Resolution", "No signal");
            ui->tableWidget_propertyTable->modify_property("Refresh rate", "No signal");
            ui->tableWidget_propertyTable->modify_property("Uptime", "No signal");
        }

        // Start timers to keep track of the video mode's uptime.
        {
            VIDEO_MODE_UPTIME.start();
            VIDEO_MODE_UPTIME_UPDATE.start(1000);

            connect(&VIDEO_MODE_UPTIME_UPDATE, &QTimer::timeout, [this]
            {
                const unsigned seconds = unsigned(VIDEO_MODE_UPTIME.elapsed() / 1000);
                const unsigned minutes = (seconds / 60);
                const unsigned hours   = (minutes / 60);

                if (kc_capture_api().has_no_signal())
                {
                    ui->tableWidget_propertyTable->modify_property("Uptime", "No signal");
                }
                else
                {
                    ui->tableWidget_propertyTable->modify_property("Uptime", QString("%1:%2:%3").arg(QString::number(hours).rightJustified(2, '0'))
                                                                                                .arg(QString::number(minutes % 60).rightJustified(2, '0'))
                                                                                                .arg(QString::number(seconds % 60).rightJustified(2, '0')));
                }
            });
        }
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "signal", this->size()).toSize());
    }

    return;
}

SignalDialog::~SignalDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "signal", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

// Called to inform the dialog that a new capture signal has been received.
void SignalDialog::notify_of_new_capture_signal(void)
{
    VIDEO_MODE_UPTIME.restart();
    update_information_table(true);

    return;
}

void SignalDialog::set_controls_enabled(const bool state)
{
    update_information_table(state);

    return;
}

void SignalDialog::update_information_table(const bool isReceivingSignal)
{
    const auto inputChannelIdx = (kc_capture_api().get_input_channel_idx() + 1);

    ui->tableWidget_propertyTable->modify_property("Input channel", QString::number(inputChannelIdx));

    if (isReceivingSignal)
    {
        const resolution_s resolution = kc_capture_api().get_resolution();
        const auto refreshRate = kc_capture_api().get_refresh_rate().value<double>();

        ui->tableWidget_propertyTable->modify_property("Refresh rate", QString("%1 Hz").arg(QString::number(refreshRate)));
        ui->tableWidget_propertyTable->modify_property("Resolution", QString("%1 x %2").arg(resolution.w)
                                                                                       .arg(resolution.h));
    }
    else
    {
        ui->tableWidget_propertyTable->modify_property("Resolution", "No signal");
        ui->tableWidget_propertyTable->modify_property("Refresh rate", "No signal");
    }

    return;
}
