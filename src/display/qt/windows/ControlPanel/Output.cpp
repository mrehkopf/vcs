#include <QVBoxLayout>
#include "display/qt/windows/ControlPanel/Output/Status.h"
#include "display/qt/windows/ControlPanel/Output/Histogram.h"
#include "display/qt/windows/ControlPanel/Output/Renderer.h"
#include "display/qt/windows/ControlPanel/Output/Overlay.h"
#include "display/qt/windows/ControlPanel/Output/Scaler.h"
#include "display/qt/windows/ControlPanel/Output/Size.h"
#include "display/qt/windows/OutputWindow.h"
#include "Output.h"
#include "ui_Output.h"

control_panel::Output::Output(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Output)
{
    ui->setupUi(this);
    this->set_name("Output");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setMargin(16);

    this->sizeDialog = new control_panel::output::Size(parent);
    this->scalerDialog = new control_panel::output::Scaler(parent);
    this->overlayDialog = new control_panel::output::Overlay(parent);
    this->rendererDialog = new control_panel::output::Renderer(parent);
    this->histogramDialog = new control_panel::output::Histogram(parent);
    this->outputStatusDialog = new control_panel::output::Status(parent);

    layout->addWidget(this->sizeDialog);
    layout->addWidget(this->outputStatusDialog);
    layout->addWidget(this->histogramDialog);
    layout->addWidget(this->scalerDialog);
    layout->addWidget(this->rendererDialog);
    layout->addWidget(this->overlayDialog);

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

control_panel::Output::~Output()
{
    delete ui;
}
