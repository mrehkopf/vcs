/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_WINDOW_H
#define VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_WINDOW_H

#include <QPushButton>
#include <QMouseEvent>
#include <QWidget>

namespace control_panel
{
    class AboutVCS;
    class Overlay;
    class Capture;
    class Output;
    class FilterGraph;
    class VideoPresets;
}

class OutputWindow;
class WheelBlocker;

/// TODO: This class should live in its own source file under src/display/qt/widgets/.
class RightClickableButton : public QPushButton
{
    Q_OBJECT

public:
    using QPushButton::QPushButton;

signals:
    void rightPressed();

private:
    void mousePressEvent(QMouseEvent *event)
    {
        if (event->button() == Qt::RightButton)
        {
            emit this->rightPressed();
        }

        QPushButton::mousePressEvent(event);
    }
};

namespace Ui { class ControlPanel; }

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(OutputWindow *parent = nullptr);
    ~ControlPanel();

    control_panel::Output* output(void) const               { return this->outputDialog; }
    control_panel::Capture* capture(void) const             { return this->captureDialog; }
    control_panel::FilterGraph* filter_graph(void) const    { return this->filterGraphDialog; }
    control_panel::VideoPresets* video_presets(void) const  { return this->videoPresetsDialog; }

private:
    void resizeEvent(QResizeEvent *);

    Ui::ControlPanel *ui;

    control_panel::Output *outputDialog = nullptr;
    control_panel::Capture *captureDialog = nullptr;
    control_panel::AboutVCS *aboutDialog = nullptr;
    control_panel::FilterGraph *filterGraphDialog = nullptr;
    control_panel::VideoPresets *videoPresetsDialog = nullptr;

    WheelBlocker *wheelBlocker;
};

#endif
