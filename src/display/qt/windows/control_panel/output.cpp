#include <QVBoxLayout>
#include "display/qt/windows/control_panel/output/renderer.h"
#include "display/qt/windows/control_panel/output/scaler.h"
#include "display/qt/windows/control_panel/output/size.h"
#include "display/qt/windows/output_window.h"
#include "output.h"
#include "ui_output.h"

control_panel::Output::Output(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Output)
{
    ui->setupUi(this);

    this->set_name("Window settings");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(9);
    layout->setMargin(9);

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
