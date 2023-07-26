#include <QVBoxLayout>
#include "display/qt/windows/control_panel/capture/input_resolution.h"
#include "display/qt/windows/control_panel/capture/input_channel.h"
#include "display/qt/windows/control_panel/capture/signal_status.h"
#include "capture/capture.h"
#include "capture.h"
#include "ui_capture.h"

CaptureDialog::CaptureDialog(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::CaptureDialog)
{
    ui->setupUi(this);

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(9);
    layout->setMargin(9);

    this->inputChannel = new InputChannel(parent);
    this->signalStatus = new SignalStatus(parent);
    this->inputResolution = new InputResolution(parent);

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

CaptureDialog::~CaptureDialog()
{
    delete ui;
}
