/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * The widget that makes up the control panel's 'Output' tab.
 *
 */

#include "display/qt/widgets/control_panel_output_widget.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "filter/anti_tear.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "ui_control_panel_output_widget.h"

ControlPanelOutputWidget::ControlPanelOutputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelOutputWidget)
{
    ui->setupUi(this);

    // Set the GUI controls to their proper initial values.
    {
        ui->spinBox_outputResX->setMinimum(MIN_OUTPUT_WIDTH);
        ui->spinBox_outputResY->setMinimum(MIN_OUTPUT_HEIGHT);
        ui->spinBox_outputResX->setMaximum(MAX_OUTPUT_WIDTH);
        ui->spinBox_outputResY->setMaximum(MAX_OUTPUT_HEIGHT);

        // Populate the lists of scaling filters.
        {
            block_widget_signals_c b1(ui->comboBox_outputUpscaleFilter);
            block_widget_signals_c b2(ui->comboBox_outputDownscaleFilter);

            ui->comboBox_outputUpscaleFilter->clear();
            ui->comboBox_outputDownscaleFilter->clear();

            const std::vector<std::string> filterNames = ks_list_of_scaling_filter_names();
            k_assert(!filterNames.empty(), "Expected to receive a list of scaling filters, but got an empty list.");

            for (const auto &name: filterNames)
            {
                ui->comboBox_outputUpscaleFilter->addItem(QString::fromStdString(name));
                ui->comboBox_outputDownscaleFilter->addItem(QString::fromStdString(name));
            }

            return;
        }

        // Restore persistent settings.
        {
            set_qcombobox_idx_c(ui->comboBox_outputUpscaleFilter)
                               .by_string(kpers_value_of(INI_GROUP_OUTPUT, "upscaler", "Linear").toString());

            set_qcombobox_idx_c(ui->comboBox_outputDownscaleFilter)
                               .by_string(kpers_value_of(INI_GROUP_OUTPUT, "downscaler", "Linear").toString());

            set_qcombobox_idx_c(ui->comboBox_outputAspectMode)
                               .by_string(kpers_value_of(INI_GROUP_OUTPUT, "aspect_mode", "Native").toString());

            set_qcombobox_idx_c(ui->comboBox_renderer)
                               .by_string(kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString());

            ui->checkBox_customFiltering->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "custom_filtering", 0).toBool());

            ui->checkBox_outputKeepAspectRatio->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "keep_aspect", true).toBool());

            ui->checkBox_forceOutputRes->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_output_size", false).toBool());

            ui->spinBox_outputResX->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().width());

            ui->spinBox_outputResY->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().height());

            ui->checkBox_forceOutputScale->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_relative_scale", false).toBool());

            ui->spinBox_outputScale->setValue(kpers_value_of(INI_GROUP_OUTPUT, "relative_scale", 100).toInt());

            ui->checkBox_outputAntiTear->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", false).toBool());
        }
    }

    return;
}

ControlPanelOutputWidget::~ControlPanelOutputWidget()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", ui->checkBox_outputAntiTear->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "upscaler", ui->comboBox_outputUpscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "downscaler", ui->comboBox_outputDownscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "custom_filtering", ui->checkBox_customFiltering->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "keep_aspect", ui->checkBox_outputKeepAspectRatio->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "aspect_mode", ui->comboBox_outputAspectMode->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "force_output_size", ui->checkBox_forceOutputRes->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "output_size", QSize(ui->spinBox_outputResX->value(), ui->spinBox_outputResY->value()));
        kpers_set_value(INI_GROUP_OUTPUT, "force_relative_scale", ui->checkBox_forceOutputScale->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "relative_scale", ui->spinBox_outputScale->value());
        kpers_set_value(INI_GROUP_OUTPUT, "renderer", ui->comboBox_renderer->currentText());
    }

    delete ui;

    return;
}

void ControlPanelOutputWidget::set_output_size_controls_enabled(const bool state)
{
    ui->frame_outputOverrides->setEnabled(state);

    return;
}

void ControlPanelOutputWidget::update_output_framerate(const uint fps,
                                                       const bool hasMissedFrames)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to update GUI output info while there was no signal."));
    }
    else
    {
        ui->label_outputFPS->setText((fps == 0)? "Unknown" : QString("%1").arg(fps));
        ui->label_outputProcessTime->setText(hasMissedFrames? "Dropping frames" : "No problem");
    }

    return;
}

// Enable/disable the output information fields.
void ControlPanelOutputWidget::set_output_info_enabled(const bool state)
{
    ui->label_outputFPS->setEnabled(state);
    ui->label_outputProcessTime->setEnabled(state);
    ui->label_outputFPS->setText("n/a");
    ui->label_outputProcessTime->setText("n/a");

    return;
}

void ControlPanelOutputWidget::update_output_resolution_info(void)
{
    const resolution_s r = ks_output_resolution();

    ui->label_outputResolution->setText(QString("%1 x %2").arg(r.w).arg(r.h));

    return;
}

void ControlPanelOutputWidget::update_capture_signal_info(void)
{
    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        const capture_signal_s s = kc_hardware().status.signal();

        ui->spinBox_outputResX->setValue(s.r.w);
        ui->spinBox_outputResY->setValue(s.r.h);
    }

    return;
}

bool ControlPanelOutputWidget::is_overlay_enabled(void)
{
    return ui->checkBox_outputOverlayEnabled->isChecked();
}

// Adjusts the output scale value in the GUI by a pre-set step size in the given
// direction. Note that this will automatically trigger a change in the actual
// scaler output size as well.
void ControlPanelOutputWidget::adjust_output_scaling(const int dir)
{
    const int curValue = ui->spinBox_outputScale->value();
    const int stepSize = ui->spinBox_outputScale->singleStep();

    k_assert((dir == 1 || dir == -1),
             "Expected the parameter for AdjustOutputScaleValue to be either 1 or -1.");

    if (!ui->checkBox_forceOutputScale->isChecked())
    {
        ui->checkBox_forceOutputScale->setChecked(true);
    }

    ui->spinBox_outputScale->setValue(curValue + (stepSize * dir));

    return;
}

QString ControlPanelOutputWidget::output_framerate_as_qstring(void)
{
    return ui->label_outputFPS->text();
}

QString ControlPanelOutputWidget::output_framedrop_as_qstring(void)
{
    return ui->label_outputProcessTime->text();
}

QString ControlPanelOutputWidget::output_latency_as_qstring(void)
{
    return ui->label_outputProcessTime->text();
}

QString ControlPanelOutputWidget::output_resolution_as_qstring(void)
{
    return ui->label_outputResolution->text();
}

void ControlPanelOutputWidget::toggle_overlay(void)
{
    ui->checkBox_outputOverlayEnabled->toggle();

    return;
}

void ControlPanelOutputWidget::on_checkBox_forceOutputScale_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'forceOutputScale'. It appears to have a third state.");

    ui->spinBox_outputScale->setEnabled(checked);

    if (checked)
    {
        const int s = ui->spinBox_outputScale->value();

        ks_set_output_scaling(s / 100.0);
    }
    else
    {
        ui->spinBox_outputScale->setValue(100);
    }

    ks_set_output_scale_override_enabled(checked);

    return;
}

void ControlPanelOutputWidget::on_checkBox_forceOutputRes_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'outputRes'. It appears to have a third state.");

    ui->spinBox_outputResX->setEnabled(checked);
    ui->spinBox_outputResY->setEnabled(checked);

    ks_set_output_resolution_override_enabled(checked);

    if (!checked)
    {
        resolution_s outr;
        const resolution_s r = kc_hardware().status.capture_resolution();

        ks_set_output_base_resolution(r, true);

        outr = ks_output_base_resolution();
        ui->spinBox_outputResX->setValue(outr.w);
        ui->spinBox_outputResY->setValue(outr.h);
    }

    return;
}

void ControlPanelOutputWidget::on_spinBox_outputScale_valueChanged(int)
{
    real scale = ui->spinBox_outputScale->value() / 100.0;

    ks_set_output_scaling(scale);

    return;
}

void ControlPanelOutputWidget::on_spinBox_outputResX_valueChanged(int)
{
    const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                            (uint)ui->spinBox_outputResY->value(), 0};

    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        return;
    }

    ks_set_output_base_resolution(r, true);

    return;
}

void ControlPanelOutputWidget::on_spinBox_outputResY_valueChanged(int)
{
    const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                            (uint)ui->spinBox_outputResY->value(), 0};

    if (!ui->checkBox_forceOutputRes->isChecked())
    {
        return;
    }

    ks_set_output_base_resolution(r, true);

    return;
}

void ControlPanelOutputWidget::on_checkBox_outputKeepAspectRatio_stateChanged(int arg1)
{
    ks_set_output_pad_override_enabled((arg1 == Qt::Checked));
    ui->comboBox_outputAspectMode->setEnabled((arg1 == Qt::Checked));

    return;
}

void ControlPanelOutputWidget::on_comboBox_outputAspectMode_currentIndexChanged(const QString &arg1)
{
    INFO(("Setting aspect mode to '%s'.", arg1.toStdString().c_str()));

    if (arg1 == "Native")
    {
        ks_set_aspect_mode(aspect_mode_e::native);
    }
    else if (arg1 == "Traditional 4:3")
    {
        ks_set_aspect_mode(aspect_mode_e::traditional_4_3);
    }
    else if (arg1 == "Always 4:3")
    {
        ks_set_aspect_mode(aspect_mode_e::always_4_3);
    }
    else
    {
        k_assert(0, "Unknown aspect mode.");
    }

    return;
}

void ControlPanelOutputWidget::on_comboBox_outputUpscaleFilter_currentIndexChanged(const QString &arg1)
{
    ks_set_upscaling_filter(arg1.toStdString());

    return;
}

void ControlPanelOutputWidget::on_comboBox_outputDownscaleFilter_currentIndexChanged(const QString &arg1)
{
    ks_set_downscaling_filter(arg1.toStdString());

    return;
}

void ControlPanelOutputWidget::on_checkBox_outputAntiTear_stateChanged(int arg1)
{
    const bool checked = (arg1 == Qt::Checked)? 1 : 0;

    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'outputAntiTear'. It appears to have a third state.");

    kat_set_anti_tear_enabled(checked);

    return;
}

void ControlPanelOutputWidget::on_pushButton_antiTearOptions_clicked(void)
{
    emit open_antitear_dialog();

    return;
}

void ControlPanelOutputWidget::on_pushButton_configureFiltering_clicked(void)
{
    emit open_filter_sets_dialog();

    return;
}

void ControlPanelOutputWidget::on_pushButton_editOverlay_clicked(void)
{
    emit open_overlay_dialog();

    return;
}

void ControlPanelOutputWidget::on_checkBox_customFiltering_stateChanged(int arg1)
{
    k_assert(arg1 != Qt::PartiallyChecked,
             "Expected a two-state toggle for 'customFiltering'. It appears to have a third state.");

    emit set_filtering_enabled((arg1 == Qt::Checked)? 1 : 0);

    return;
}

void ControlPanelOutputWidget::on_comboBox_renderer_currentIndexChanged(const QString &rendererName)
{
    emit set_renderer(rendererName);

    return;
}
