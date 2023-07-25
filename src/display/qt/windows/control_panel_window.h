#ifndef VCS_DISPLAY_QT_DIALOGS_CONTROL_PANEL_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_CONTROL_PANEL_DIALOG_H

#include <QWidget>

class OutputWindow;
class CaptureDialog;
class WindowOptionsDialog;
class FilterGraphDialog;
class VideoPresetsDialog;
class OverlayDialog;
class AboutDialog;

namespace Ui {
class ControlPanelWindow;
}

class ControlPanelWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelWindow(OutputWindow *parent = nullptr);
    ~ControlPanelWindow();

    CaptureDialog* capture(void)              const { return this->captureDialog; }
    FilterGraphDialog* filter_graph(void)     const { return this->filterGraphDialog; }
    VideoPresetsDialog* video_presets(void)   const { return this->videoPresetsDialog; }
    WindowOptionsDialog* window_options(void) const { return this->windowOptionsDialog; }
    OverlayDialog* overlay(void)              const { return this->overlayDialog; }

private:
    Ui::ControlPanelWindow *ui;

    CaptureDialog *captureDialog = nullptr;
    WindowOptionsDialog *windowOptionsDialog = nullptr;
    FilterGraphDialog *filterGraphDialog = nullptr;
    VideoPresetsDialog *videoPresetsDialog = nullptr;
    OverlayDialog *overlayDialog = nullptr;
    AboutDialog *aboutDialog = nullptr;
};

#endif
