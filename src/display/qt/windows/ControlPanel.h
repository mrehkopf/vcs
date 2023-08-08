#ifndef VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_WINDOW_H
#define VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_WINDOW_H

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

namespace Ui { class ControlPanel; }

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(OutputWindow *parent = nullptr);
    ~ControlPanel();

    control_panel::Capture* capture(void) const { return this->captureDialog; }
    control_panel::FilterGraph* filter_graph(void)     const { return this->filterGraphDialog; }
    control_panel::VideoPresets* video_presets(void)   const { return this->videoPresetsDialog; }
    control_panel::Output* output(void) const { return this->outputDialog; }
    control_panel::Overlay* overlay(void)              const { return this->overlayDialog; }

private:
    Ui::ControlPanel *ui;

    control_panel::Capture *captureDialog = nullptr;
    control_panel::Output *outputDialog = nullptr;
    control_panel::FilterGraph *filterGraphDialog = nullptr;
    control_panel::VideoPresets *videoPresetsDialog = nullptr;
    control_panel::Overlay *overlayDialog = nullptr;
    control_panel::AboutVCS *aboutDialog = nullptr;
};

#endif
