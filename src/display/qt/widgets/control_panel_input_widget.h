#ifndef CONTROL_PANEL_INPUT_WIDGET_H
#define CONTROL_PANEL_INPUT_WIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelInputWidget;
}

class ControlPanelInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelInputWidget(QWidget *parent = 0);
    ~ControlPanelInputWidget();

private:
    Ui::ControlPanelInputWidget *ui;
};

#endif // CONTROL_PANEL_INPUT_WIDGET_H
