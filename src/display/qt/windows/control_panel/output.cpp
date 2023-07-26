#include <QVBoxLayout>
#include "display/qt/windows/control_panel/output/output_renderer.h"
#include "display/qt/windows/control_panel/output/output_scaler.h"
#include "display/qt/windows/control_panel/output/output_size.h"
#include "display/qt/windows/output_window.h"
#include "output.h"
#include "ui_output.h"

WindowOptionsDialog::WindowOptionsDialog(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::WindowOptionsDialog)
{
    ui->setupUi(this);

    this->set_name("Window settings");

    auto *const layout = new QVBoxLayout(this);
    layout->setSpacing(9);
    layout->setMargin(9);

    this->windowSize = new WindowSize(parent);
    this->windowScaler = new WindowScaler(parent);
    this->windowRenderer = new WindowRenderer(parent);

    layout->addWidget(this->windowSize);
    layout->addWidget(this->windowScaler);
    layout->addWidget(this->windowRenderer);

    // Push out empty space from the dialogs.
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

WindowOptionsDialog::~WindowOptionsDialog()
{
    delete ui;
}
