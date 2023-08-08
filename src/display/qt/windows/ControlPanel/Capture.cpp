#include <QVBoxLayout>
#include "display/qt/windows/ControlPanel/Capture/InputResolution.h"
#include "display/qt/windows/ControlPanel/Capture/InputChannel.h"
#include "display/qt/windows/ControlPanel/Capture/SignalStatus.h"
#include "capture/capture.h"
#include "Capture.h"
#include "ui_Capture.h"

control_panel::Capture::Capture(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Capture)
{
    ui->setupUi(this);

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setMargin(16);

    this->inputChannel = new control_panel::capture::InputChannel(parent);
    this->signalStatus = new control_panel::capture::SignalStatus(parent);
    this->inputResolution = new control_panel::capture::InputResolution(parent);

    // Compose the dialog's layout.
    {
        if (kc_device_property("supports channel switching: ui"))
        {
            layout->addWidget(this->inputChannel);
        }
        else
        {
            this->inputChannel->setEnabled(false);
        }

        layout->addWidget(this->signalStatus);

        if (kc_device_property("supports resolution switching: ui"))
        {
            layout->addWidget(this->inputResolution);
        }
        else
        {
            this->inputResolution->setEnabled(false);
        }
    }

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 9999999, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

control_panel::Capture::~Capture()
{
    delete ui;
}
