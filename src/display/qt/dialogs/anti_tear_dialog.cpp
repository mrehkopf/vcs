/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear dialog
 *
 * Provides GUI controls for the user to customize the anti-tear engine's
 * parameters.
 *
 */

#include <QMenuBar>
#include "display/qt/subclasses/QGroupBox_parameter_grid.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/propagate/app_events.h"
#include "anti_tear/anti_tear.h"
#include "capture/capture.h"
#include "capture/capture_device.h"
#include "display/display.h"
#include "common/globals.h"
#include "ui_anti_tear_dialog.h"

AntiTearDialog::AntiTearDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::AntiTearDialog)
{
    ui->setupUi(this);

    this->set_name("Anti-tear");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create the dialog's menu bar.
    {
        this->menuBar = new QMenuBar(this);

        // Anti-tear...
        {
            QMenu *antitearMenu = new QMenu("Anti-tear", this->menuBar);

            QAction *enable = new QAction("Enabled", this->menuBar);
            enable->setCheckable(true);
            enable->setChecked(this->is_enabled());

            connect(this, &VCSBaseDialog::enabled_state_set, this, [=](const bool isEnabled)
            {
                enable->setChecked(isEnabled);
            });

            connect(enable, &QAction::triggered, this, [=]
            {
                this->set_enabled(!this->is_enabled());
            });

            antitearMenu->addAction(enable);

            this->menuBar->addMenu(antitearMenu);
        }


        this->layout()->setMenuBar(this->menuBar);
    }

    // Set GUI controls to their initial values.
    {
        ui->checkBox_visualizeRange->setChecked(KAT_DEFAULT_VISUALIZE_SCAN_RANGE);
        ui->checkBox_visualizeTear->setChecked(KAT_DEFAULT_VISUALIZE_TEARS);

        ui->parameterGrid_parameters->add_combobox("Scan direction", {"Down",
                                                                      "Up"});
        ui->parameterGrid_parameters->add_combobox("Scan hint", {"Look for one tear per frame",
                                                                 "Look for multiple tears per frame"});
        ui->parameterGrid_parameters->add_separator();
        ui->parameterGrid_parameters->add_scroller("Scan start", 0, 0, 640);
        ui->parameterGrid_parameters->add_scroller("Scan end", 0, 0, 640);
        ui->parameterGrid_parameters->add_separator();
        ui->parameterGrid_parameters->add_scroller("Threshold", KAT_DEFAULT_THRESHOLD, 0, 255);
        ui->parameterGrid_parameters->add_scroller("Window size", KAT_DEFAULT_WINDOW_LENGTH, 1, 640);
        ui->parameterGrid_parameters->add_scroller("Step size", KAT_DEFAULT_STEP_SIZE, 1, 640);
        ui->parameterGrid_parameters->add_scroller("Matches req'd", KAT_DEFAULT_NUM_MATCHES_REQUIRED, 1, 200);
    }

    // Subscribe to app events.
    {
        ke_events().capture.newVideoMode.subscribe([this]
        {
            const auto resolution = kc_capture_device().get_resolution();

            ui->parameterGrid_parameters->set_maximum_value("Scan start", (resolution.h - 1));
            ui->parameterGrid_parameters->set_maximum_value("Scan end", (resolution.h - 1));
            ui->parameterGrid_parameters->set_maximum_value("Window size", 99);
            ui->parameterGrid_parameters->set_maximum_value("Step size", 99);
        });
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(this, &AntiTearDialog::enabled_state_set, this, [this](const bool isEnabled)
        {
            kat_set_anti_tear_enabled(isEnabled);
            kd_update_output_window_title();

            ui->parameterGrid_parameters->setEnabled(isEnabled);
            ui->groupBox_visualization->setEnabled(isEnabled);
            this->update_visualization_options();
        });

        connect(ui->parameterGrid_parameters, &ParameterGrid::parameter_value_changed, this, [this](const QString &parameterName)
        {
            const auto newValue = ui->parameterGrid_parameters->value(parameterName);

            if (parameterName == "Scan start") kat_set_range(newValue, ui->parameterGrid_parameters->value("Scan end"));
            else if (parameterName == "Scan end") kat_set_range(ui->parameterGrid_parameters->value("Scan start"), newValue);
            else if (parameterName == "Threshold") kat_set_threshold(newValue);
            else if (parameterName == "Window size") kat_set_domain_size(newValue);
            else if (parameterName == "Step size") kat_set_step_size(newValue);
            else if (parameterName == "Matches req'd") kat_set_matches_required(newValue);
            else if (parameterName == "Scan hint")
            {
                switch (newValue)
                {
                    case 0: kat_set_scan_hint(anti_tear_scan_hint_e::look_for_one_tear); break;
                    case 1: kat_set_scan_hint(anti_tear_scan_hint_e::look_for_multiple_tears); break;
                    default: k_assert(0, "Unknown scan method."); break;
                }
            }
            else if (parameterName == "Scan direction")
            {
                switch (newValue)
                {
                    case 0: kat_set_scan_direction(anti_tear_scan_direction_e::down); break;
                    case 1: kat_set_scan_direction(anti_tear_scan_direction_e::up); break;
                    default: k_assert(0, "Unknown scan direction."); break;
                }
            }
            else k_assert(0, "Unknown parameter identifier");
        });

        connect(ui->checkBox_visualizeRange, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });

        connect(ui->checkBox_visualizeTear, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });
    }

    // Restore persistent settings.
    {
        ui->parameterGrid_parameters->set_value("Scan start", kpers_value_of(INI_GROUP_ANTI_TEAR, "scan_start", 0).toInt());
        ui->parameterGrid_parameters->set_value("Scan end", kpers_value_of(INI_GROUP_ANTI_TEAR, "scan_end", 0).toInt());
        ui->parameterGrid_parameters->set_value("Threshold", kpers_value_of(INI_GROUP_ANTI_TEAR, "threshold", KAT_DEFAULT_THRESHOLD).toInt());
        ui->parameterGrid_parameters->set_value("Window size", kpers_value_of(INI_GROUP_ANTI_TEAR, "window_len", KAT_DEFAULT_WINDOW_LENGTH).toInt());
        ui->parameterGrid_parameters->set_value("Step size", kpers_value_of(INI_GROUP_ANTI_TEAR, "step_size", KAT_DEFAULT_STEP_SIZE).toInt());
        ui->parameterGrid_parameters->set_value("Matches req'd", kpers_value_of(INI_GROUP_ANTI_TEAR, "matches_reqd", KAT_DEFAULT_NUM_MATCHES_REQUIRED).toInt());
        //ui->parameterGrid_parameters->set_value("Scan direction", kpers_value_of(INI_GROUP_ANTI_TEAR, "scan_direction", KAT_DEFAULT_SCAN_DIRECTION).toInt());
        //ui->parameterGrid_parameters->set_value("Scan hint", kpers_value_of(INI_GROUP_ANTI_TEAR, "scan_hint", KAT_DEFAULT_SCAN_HINT).toInt());
        ui->checkBox_visualizeRange->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "visualize_range", KAT_DEFAULT_VISUALIZE_SCAN_RANGE).toBool());
        ui->checkBox_visualizeTear->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "visualize_tear", KAT_DEFAULT_VISUALIZE_TEARS).toBool());
        this->set_enabled(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", this->is_enabled()).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "anti_tear", this->size()).toSize());
    }

    this->update_visualization_options();

    return;
}

AntiTearDialog::~AntiTearDialog(void)
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "anti_tear", this->size());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "scan_end", ui->parameterGrid_parameters->value("Scan end"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "scan_start", ui->parameterGrid_parameters->value("Scan start"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "threshold", ui->parameterGrid_parameters->value("Threshold"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "window_len", ui->parameterGrid_parameters->value("Window size"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "matches_reqd", ui->parameterGrid_parameters->value("Matches req'd"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "step_size", ui->parameterGrid_parameters->value("Step size"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "visualize_range", ui->checkBox_visualizeRange->isChecked());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "visualize_tear", ui->checkBox_visualizeTear->isChecked());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "direction", 0);
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", this->is_enabled());
    }

    delete ui;

    return;
}

void AntiTearDialog::update_visualization_options(void)
{
    kat_set_visualization(ui->checkBox_visualizeTear->isChecked(),
                          ui->checkBox_visualizeRange->isChecked());

    return;
}
