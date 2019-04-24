#include "control_panel_input_widget.h"
#include "ui_control_panel_input_widget.h"

ControlPanelInputWidget::ControlPanelInputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelInputWidget)
{
    ui->setupUi(this);
}

ControlPanelInputWidget::~ControlPanelInputWidget()
{
    delete ui;
}
