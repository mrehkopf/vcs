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

    bool is_custom_program_styling_enabled(void);

    void restore_persistent_settings(void);

    void notify_of_new_program_version(void);

signals:
    void new_programwide_style_file(const QString &filename);

private:
    Ui::ControlPanelAboutWidget *ui;
};

#endif
