#ifndef CONTROL_PANEL_OUTPUT_WIDGET_H
#define CONTROL_PANEL_OUTPUT_WIDGET_H

#include <QWidget>

namespace Ui {
class ControlPanelOutputWidget;
}

class ControlPanelOutputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanelOutputWidget(QWidget *parent = 0);
    ~ControlPanelOutputWidget();

    void set_output_size_controls_enabled(const bool state);

    void update_output_framerate(const uint fps, const bool hasMissedFrames);

    void set_output_info_enabled(const bool state);

    void update_output_resolution_info(void);

    void update_capture_signal_info(void);

    bool is_overlay_enabled();

    void adjust_output_scaling(const int dir);

    void toggle_overlay();

private slots:
    void on_checkBox_forceOutputScale_stateChanged(int arg1);

    void on_checkBox_forceOutputRes_stateChanged(int arg1);

    void on_spinBox_outputScale_valueChanged(int);

    void on_spinBox_outputResX_valueChanged(int);

    void on_spinBox_outputResY_valueChanged(int);

    void on_checkBox_outputKeepAspectRatio_stateChanged(int arg1);

    void on_comboBox_outputAspectMode_currentIndexChanged(const QString &arg1);

    void on_comboBox_outputUpscaleFilter_currentIndexChanged(const QString &arg1);

    void on_comboBox_outputDownscaleFilter_currentIndexChanged(const QString &arg1);

    void on_pushButton_antiTearOptions_clicked(void);

    void on_pushButton_configureFiltering_clicked(void);

    void on_checkBox_customFiltering_stateChanged(int arg1);

    void on_checkBox_outputAntiTear_stateChanged(int arg1);

    void on_pushButton_editOverlay_clicked(void);

    void on_comboBox_renderer_currentIndexChanged(const QString &rendererName);

signals:
    // Ask the control panel to open the anti-tear dialog.
    void open_antitear_dialog();

    // Ask the control panel to open the filter sets dialog.
    void open_filter_sets_dialog();

    // Ask the control panel to open the overlay dialog.
    void open_overlay_dialog();

    // Signal that the user has chosen to enable/disable custom output filtering.
    void set_filtering_enabled(const bool state);

    // Signal that the user has activated the renderer of the given name.
    void set_renderer(const QString &rendererName);

private:
    Ui::ControlPanelOutputWidget *ui;
};

#endif
