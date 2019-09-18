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

    // Initialize the GUI controls.
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
        }
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        // For overloaded signals. I'm using Qt 5.5, but you may have better ways
        // of doing this in later versions.
        #define QCOMBOBOX_QSTRING static_cast<void (QComboBox::*)(const QString&)>
        #define QSPINBOX_INT static_cast<void (QSpinBox::*)(int)>

        // Returns true if the given checkbox is in a proper checked state.
        // Returns false if it's in a proper unchecked state; and assert fails
        // if the checkbox is neither fully checked nor fully unchecked.
        auto is_checked = [](int qcheckboxCheckedState)
        {
            k_assert(qcheckboxCheckedState != Qt::PartiallyChecked,
                     "Expected this QCheckBox to have a two-state toggle. It appears to have a third state.");

            return ((qcheckboxCheckedState == Qt::Checked)? true : false);
        };

        connect(ui->checkBox_outputAntiTearEnabled, &QCheckBox::stateChanged, this,
                [=](int state){ kat_set_anti_tear_enabled(is_checked(state)); });

        connect(ui->checkBox_outputFilteringEnabled, &QCheckBox::stateChanged, this,
                [=](int state){ emit set_filtering_enabled(is_checked(state)); });

        connect(ui->pushButton_antiTearOptions, &QPushButton::clicked, this,
                [this]{ emit open_antitear_dialog(); });

        connect(ui->pushButton_configureFiltering, &QPushButton::clicked, this,
                [this]{ emit open_filter_graph_dialog(); });

        connect(ui->pushButton_editOverlay, &QPushButton::clicked, this, [this]
                { emit open_overlay_dialog(); });

        connect(ui->checkBox_forceOutputScale, &QCheckBox::stateChanged, this, [=](int state)
        {
            if (is_checked(state))
            {
                ks_set_output_scaling(ui->spinBox_outputScale->value() / 100.0);
            }
            else
            {
                ui->spinBox_outputScale->setValue(100);
            }

            ui->spinBox_outputScale->setEnabled(is_checked(state));
            ks_set_output_scale_override_enabled(is_checked(state));
        });

        connect(ui->checkBox_forceOutputRes, &QCheckBox::stateChanged, this, [=](int state)
        {
            ui->spinBox_outputResX->setEnabled(is_checked(state));
            ui->spinBox_outputResY->setEnabled(is_checked(state));

            ks_set_output_resolution_override_enabled(is_checked(state));

            if (!is_checked(state))
            {
                ks_set_output_base_resolution(kc_hardware().status.capture_resolution(), true);

                ui->spinBox_outputResX->setValue(ks_output_base_resolution().w);
                ui->spinBox_outputResY->setValue(ks_output_base_resolution().h);
            }
        });

        connect(ui->checkBox_outputKeepAspectRatio, &QCheckBox::stateChanged, this, [=](int state)
        {
            ks_set_forced_aspect_enabled(is_checked(state));
            ui->comboBox_outputAspectMode->setEnabled(is_checked(state));
        });

        connect(ui->spinBox_outputResX, QSPINBOX_INT(&QSpinBox::valueChanged), this,
                [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                                    (uint)ui->spinBox_outputResY->value(), 0};

            ks_set_output_base_resolution(r, true);
        });

        connect(ui->spinBox_outputResY, QSPINBOX_INT(&QSpinBox::valueChanged), this,
                [this]
        {
            if (!ui->checkBox_forceOutputRes->isChecked()) return;

            const resolution_s r = {(uint)ui->spinBox_outputResX->value(),
                                    (uint)ui->spinBox_outputResY->value(), 0};

            ks_set_output_base_resolution(r, true);
        });

        connect(ui->spinBox_outputScale, QSPINBOX_INT(&QSpinBox::valueChanged), this,
                [this]{ ks_set_output_scaling(ui->spinBox_outputScale->value() / 100.0); });

        connect(ui->comboBox_outputUpscaleFilter, QCOMBOBOX_QSTRING(&QComboBox::currentIndexChanged), this,
                [this](const QString &upscalerName) { ks_set_upscaling_filter(upscalerName.toStdString()); });

        connect(ui->comboBox_outputDownscaleFilter, QCOMBOBOX_QSTRING(&QComboBox::currentIndexChanged), this,
                [this](const QString &downscalerName) { ks_set_downscaling_filter(downscalerName.toStdString()); });

        connect(ui->comboBox_renderer, QCOMBOBOX_QSTRING(&QComboBox::currentIndexChanged), this,
                [this](const QString &rendererName) { emit set_renderer(rendererName); });

        connect(ui->comboBox_outputAspectMode, QCOMBOBOX_QSTRING(&QComboBox::currentIndexChanged), this,
                [this](const QString &aspectModeString)
        {
            INFO(("Setting aspect mode to '%s'.", aspectModeString.toStdString().c_str()));

            if (aspectModeString == "Native")
            {
                ks_set_aspect_mode(aspect_mode_e::native);
            }
            else if (aspectModeString == "Traditional 4:3")
            {
                ks_set_aspect_mode(aspect_mode_e::traditional_4_3);
            }
            else if (aspectModeString == "Always 4:3")
            {
                ks_set_aspect_mode(aspect_mode_e::always_4_3);
            }
            else
            {
                k_assert(0, "Unknown aspect mode.");
            }
        });

        #undef QCOMBOBOX_QSTRING
        #undef QSPINBOX_INT
    }

    return;
}

ControlPanelOutputWidget::~ControlPanelOutputWidget()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", ui->checkBox_outputAntiTearEnabled->isChecked());
        kpers_set_value(INI_GROUP_OVERLAY, "enabled", ui->checkBox_outputOverlayEnabled->isChecked());
        kpers_set_value(INI_GROUP_OUTPUT, "upscaler", ui->comboBox_outputUpscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "downscaler", ui->comboBox_outputDownscaleFilter->currentText());
        kpers_set_value(INI_GROUP_OUTPUT, "custom_filtering", ui->checkBox_outputFilteringEnabled->isChecked());
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

void ControlPanelOutputWidget::restore_persistent_settings(void)
{
    set_qcombobox_idx_c(ui->comboBox_outputUpscaleFilter)
                       .by_string(kpers_value_of(INI_GROUP_OUTPUT, "upscaler", "Linear").toString());

    set_qcombobox_idx_c(ui->comboBox_outputDownscaleFilter)
                       .by_string(kpers_value_of(INI_GROUP_OUTPUT, "downscaler", "Linear").toString());

    set_qcombobox_idx_c(ui->comboBox_outputAspectMode)
                       .by_string(kpers_value_of(INI_GROUP_OUTPUT, "aspect_mode", "Native").toString());

    set_qcombobox_idx_c(ui->comboBox_renderer)
                       .by_string(kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString());

    ui->checkBox_outputFilteringEnabled->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "custom_filtering", 0).toBool());
    ui->checkBox_outputKeepAspectRatio->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "keep_aspect", true).toBool());
    ui->checkBox_outputOverlayEnabled->setChecked(kpers_value_of(INI_GROUP_OVERLAY, "enabled", true).toBool());
    ui->checkBox_forceOutputRes->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_output_size", false).toBool());
    ui->spinBox_outputResX->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().width());
    ui->spinBox_outputResY->setValue(kpers_value_of(INI_GROUP_OUTPUT, "output_size", QSize(640, 480)).toSize().height());
    ui->checkBox_forceOutputScale->setChecked(kpers_value_of(INI_GROUP_OUTPUT, "force_relative_scale", false).toBool());
    ui->spinBox_outputScale->setValue(kpers_value_of(INI_GROUP_OUTPUT, "relative_scale", 100).toInt());
    ui->checkBox_outputAntiTearEnabled->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", false).toBool());

    return;
}

void ControlPanelOutputWidget::set_output_size_controls_enabled(const bool state)
{
    ui->groupBox_outputOverrides->setEnabled(state);

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

void ControlPanelOutputWidget::toggle_overlay(void)
{
    ui->checkBox_outputOverlayEnabled->toggle();

    return;
}
