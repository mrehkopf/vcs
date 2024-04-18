#include <QVBoxLayout>
#include "display/qt/windows/ControlPanel/Capture/AliasResolutions.h"
#include "display/qt/windows/ControlPanel/Capture/InputResolution.h"
#include "display/qt/windows/ControlPanel/Capture/InputChannel.h"
#include "display/qt/windows/ControlPanel/Capture/SignalStatus.h"
#include "display/qt/windows/ControlPanel/Capture/CaptureRate.h"
#include "capture/capture.h"
#include "Capture.h"
#include "ui_Capture.h"

control_panel::Capture::Capture(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Capture)
{
    ui->setupUi(this);
    this->set_name("Capture");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(QMargins(16,16,16,16));

    this->inputChannel = new control_panel::capture::InputChannel(parent);
    this->signalStatus = new control_panel::capture::SignalStatus(parent);
    this->inputResolution = new control_panel::capture::InputResolution(parent);
    this->aliasResolutions = new control_panel::capture::AliasResolutions(parent);

    // Compose the dialog's layout.
    {
        if (kc_device_property("supports channel switching"))
        {
            layout->addWidget(this->inputChannel);
        }
        else
        {
            this->inputChannel->setEnabled(false);
        }

        layout->addWidget(this->signalStatus);

        if (kc_device_property("supports capture rate switching"))
        {
            this->captureRate = new control_panel::capture::CaptureRate(parent);
            layout->addWidget(this->captureRate);
        }

        if (kc_device_property("supports resolution switching"))
        {
            layout->addWidget(this->inputResolution);
            layout->addWidget(this->aliasResolutions);
        }
        else
        {
            this->inputResolution->setEnabled(false);
            this->aliasResolutions->setEnabled(false);
        }
    }

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 9999999, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

control_panel::Capture::~Capture()
{
    delete ui;
}
