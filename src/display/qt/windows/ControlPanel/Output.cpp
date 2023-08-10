#include <QVBoxLayout>
#include "display/qt/windows/ControlPanel/Output/Renderer.h"
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

    this->set_name("Window settings");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setMargin(16);

    this->sizeDialog = new control_panel::output::Size(parent);
    this->scalerDialog = new control_panel::output::Scaler(parent);
    this->rendererDialog = new control_panel::output::Renderer(parent);

    layout->addWidget(this->sizeDialog);
    layout->addWidget(this->scalerDialog);
    layout->addWidget(this->rendererDialog);

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

control_panel::Output::~Output()
{
    delete ui;
}