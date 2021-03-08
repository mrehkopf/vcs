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
#include "filter/anti_tear.h"
#include "capture/capture.h"
#include "capture/capture_api.h"
#include "display/display.h"
#include "common/globals.h"
#include "ui_anti_tear_dialog.h"

AntiTearDialog::AntiTearDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AntiTearDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Anti-tear");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Anti-tear...
        {
            QMenu *antitearMenu = new QMenu("Anti-tear", this->menubar);

            QAction *enable = new QAction("Enabled", this->menubar);
            enable->setCheckable(true);
            enable->setChecked(this->isEnabled);

            connect(this, &AntiTearDialog::anti_tear_enabled, this, [=]
            {
                enable->setChecked(true);
                update_visualization_options();

                ui->parameterGrid_parameters->setEnabled(true);
                ui->groupBox_visualization->setEnabled(true);
            });

            connect(this, &AntiTearDialog::anti_tear_disabled, this, [=]
            {
                enable->setChecked(false);
                update_visualization_options();

                ui->parameterGrid_parameters->setEnabled(false);
                ui->groupBox_visualization->setEnabled(false);
            });

            connect(enable, &QAction::triggered, this, [=]
            {
                this->set_anti_tear_enabled(!this->isEnabled);
            });

            antitearMenu->addAction(enable);

            this->menubar->addMenu(antitearMenu);
        }


        this->layout()->setMenuBar(this->menubar);
    }

    // Set GUI controls to their initial values.
    {
        // Parameters.
        {
            const auto defaults = kat_default_settings();

            ui->parameterGrid_parameters->add_parameter("Scan start", defaults.rangeUp, 0, 640);
            ui->parameterGrid_parameters->add_parameter("Scan end", defaults.rangeDown, 0, 640);
            ui->parameterGrid_parameters->add_spacer();
            ui->parameterGrid_parameters->add_parameter("Threshold", defaults.threshold, 0, 255);
            ui->parameterGrid_parameters->add_parameter("Window size", defaults.windowLen, 0, 640);
            ui->parameterGrid_parameters->add_parameter("Step size", defaults.stepSize, 0, 640);
            ui->parameterGrid_parameters->add_parameter("Matches req'd", defaults.matchesReqd, 0, 99);
        }
    }

    // Subscribe to app events.
    {
        ke_events().capture.newVideoMode.subscribe([this]
        {
            const auto screenHeight = kc_capture_api().get_resolution().h;

            ui->parameterGrid_parameters->set_maximum_value("Scan start", screenHeight);
            ui->parameterGrid_parameters->set_maximum_value("Scan end", screenHeight);
            ui->parameterGrid_parameters->set_maximum_value("Window size", screenHeight);
            ui->parameterGrid_parameters->set_maximum_value("Step size", screenHeight);
        });
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->parameterGrid_parameters, &ParameterGrid::parameter_value_changed, this, [this](const QString &parameterName)
        {
            const auto newValue = ui->parameterGrid_parameters->value(parameterName);

            if (parameterName == "Scan start") kat_set_range(newValue, ui->parameterGrid_parameters->value("Scan end"));
            else if (parameterName == "Scan end") kat_set_range(ui->parameterGrid_parameters->value("Scan start"), newValue);
            else if (parameterName == "Threshold") kat_set_threshold(newValue);
            else if (parameterName == "Window size") kat_set_domain_size(newValue);
            else if (parameterName == "Step size") kat_set_step_size(newValue);
            else if (parameterName == "Matches req'd") kat_set_matches_required(newValue);
            else k_assert(0, "Unknown parameter identifier");
        });

        connect(ui->checkBox_visualizeRange, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });

        connect(ui->checkBox_visualizeTear, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });
    }

    // Restore persistent settings.
    {
        const anti_tear_options_s defaults = kat_default_settings();

        ui->parameterGrid_parameters->set_value("Scan start", kpers_value_of(INI_GROUP_ANTI_TEAR, "range_up", defaults.rangeUp).toInt());
        ui->parameterGrid_parameters->set_value("Scan end", kpers_value_of(INI_GROUP_ANTI_TEAR, "range_down", defaults.rangeDown).toInt());
        ui->parameterGrid_parameters->set_value("Threshold", kpers_value_of(INI_GROUP_ANTI_TEAR, "threshold", defaults.threshold).toInt());
        ui->parameterGrid_parameters->set_value("Window size", kpers_value_of(INI_GROUP_ANTI_TEAR, "window_len", defaults.windowLen).toInt());
        ui->parameterGrid_parameters->set_value("Step size", kpers_value_of(INI_GROUP_ANTI_TEAR, "step_size", defaults.windowLen).toInt());
        ui->parameterGrid_parameters->set_value("Matches req'd", kpers_value_of(INI_GROUP_ANTI_TEAR, "matches_reqd", defaults.matchesReqd).toInt());
        ui->checkBox_visualizeRange->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "visualize_range", true).toBool());
        ui->checkBox_visualizeTear->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "visualize_tear", true).toBool());
        this->set_anti_tear_enabled(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", kat_is_anti_tear_enabled()).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "anti_tear", this->size()).toSize());
    }

    update_visualization_options();

    return;
}

AntiTearDialog::~AntiTearDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "anti_tear", this->size());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_up", ui->parameterGrid_parameters->value("Scan start"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_down", ui->parameterGrid_parameters->value("Scan end"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "threshold", ui->parameterGrid_parameters->value("Threshold"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "window_len", ui->parameterGrid_parameters->value("Window size"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "matches_reqd", ui->parameterGrid_parameters->value("Matches req'd"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "step_size", ui->parameterGrid_parameters->value("Step size"));
        kpers_set_value(INI_GROUP_ANTI_TEAR, "visualize_range", ui->checkBox_visualizeRange->isChecked());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "visualize_tear", ui->checkBox_visualizeTear->isChecked());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "direction", 0);
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", this->isEnabled);
    }

    delete ui;

    return;
}

void AntiTearDialog::update_visualization_options(void)
{
    kat_set_visualization(this->isEnabled,
                          ui->checkBox_visualizeTear->isChecked(),
                          ui->checkBox_visualizeRange->isChecked());

    return;
}

bool AntiTearDialog::is_anti_tear_enabled(void)
{
    return this->isEnabled;
}

void AntiTearDialog::set_anti_tear_enabled(const bool enabled)
{
    this->isEnabled = enabled;

    kat_set_anti_tear_enabled(isEnabled);

    if (!this->isEnabled)
    {
        emit this->anti_tear_disabled();
    }
    else
    {
        emit this->anti_tear_enabled();
    }

    kd_update_output_window_title();

    return;
}
