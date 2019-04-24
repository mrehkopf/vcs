#ifndef CONTROL_PANEL_ABOUT_WIDGET_H
#define CONTROL_PANEL_ABOUT_WIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelAboutWidget;
}

class ControlPanelAboutWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelAboutWidget(QWidget *parent = 0);
    ~ControlPanelAboutWidget();

    bool custom_program_styling_enabled();

signals:
    void new_programwide_style_file(const QString &filename);

private slots:
    void on_comboBox_customInterfaceStyling_currentIndexChanged(const QString &styleName);

private:
    Ui::ControlPanelAboutWidget *ui;
};

#endif
