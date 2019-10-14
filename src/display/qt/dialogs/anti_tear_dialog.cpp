/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear dialog
 *
 * Provides GUI controls for the user to customize the anti-tear engine's
 * parameters.
 *
 */

#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "filter/anti_tear.h"
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

    // Initialize the GUI controls to their default values.
    {
        ui->groupBox_antiTearingEnabled->setChecked(false);
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        // The valueChanged() signal is overloaded for int and QString, and we
        // have to choose one. I'm using Qt 5.5, but you may have better ways
        // of doing this in later versions.
        #define OVERLOAD_INT static_cast<void (QSpinBox::*)(int)>

        connect(ui->spinBox_rangeUp, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_range(ui->spinBox_rangeUp->value(), ui->spinBox_rangeDown->value()); });

        connect(ui->spinBox_rangeDown, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_range(ui->spinBox_rangeUp->value(), ui->spinBox_rangeDown->value()); });

        connect(ui->spinBox_threshold, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_threshold(ui->spinBox_threshold->value()); });

        connect(ui->spinBox_domainSize, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_domain_size(ui->spinBox_domainSize->value()); });

        connect(ui->spinBox_matchesReqd, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_matches_required(ui->spinBox_matchesReqd->value()); });

        connect(ui->spinBox_stepSize, OVERLOAD_INT(&QSpinBox::valueChanged), this,
                [this]{ kat_set_step_size(ui->spinBox_stepSize->value()); });

        connect(ui->groupBox_antiTearingEnabled, &QGroupBox::toggled, this,
                [this](const bool isEnabled){ kat_set_anti_tear_enabled(isEnabled); kd_update_output_window_title(); });

        #undef OVERLOAD_INT

        const auto restore_default_settings = [this]
        {
            const anti_tear_options_s defaults = kat_default_settings();

            kat_set_buffer_updates_disabled(true);
            ui->spinBox_threshold->setValue(defaults.threshold);
            ui->spinBox_matchesReqd->setValue(defaults.matchesReqd);
            ui->spinBox_domainSize->setValue(defaults.windowLen);
            ui->spinBox_stepSize->setValue(defaults.stepSize);
            kat_set_buffer_updates_disabled(false);

            return;
        };

        connect(ui->pushButton_resetDefaults, &QPushButton::clicked, this,
                [=]{ restore_default_settings(); });

        const auto update_visualization_options = [this]
        {
            kat_set_visualization(ui->groupBox_visualization->isChecked(),
                                  ui->checkBox_visualizeTear->isChecked(),
                                  ui->checkBox_visualizeRange->isChecked());

            return;
        };

        connect(ui->groupBox_visualization, &QGroupBox::toggled, this,
                [=]{ update_visualization_options(); });

        connect(ui->checkBox_visualizeRange, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });

        connect(ui->checkBox_visualizeTear, &QCheckBox::stateChanged, this,
                [=]{ update_visualization_options(); });
    }

    // Restore persistent settings.
    {
        const anti_tear_options_s defaults = kat_default_settings();

        ui->spinBox_rangeUp->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "range_up", defaults.rangeUp).toInt());
        ui->spinBox_rangeDown->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "range_down", defaults.rangeDown).toInt());
        ui->spinBox_threshold->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "threshold", defaults.threshold).toInt());
        ui->spinBox_matchesReqd->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "matches_reqd", defaults.matchesReqd).toInt());
        ui->spinBox_domainSize->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "window_len", defaults.windowLen).toInt());
        ui->groupBox_antiTearingEnabled->setChecked(kpers_value_of(INI_GROUP_ANTI_TEAR, "enabled", kat_is_anti_tear_enabled()).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "anti_tear", this->size()).toSize());
    }

    return;
}

AntiTearDialog::~AntiTearDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "anti_tear", this->size());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_up", ui->spinBox_rangeUp->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_down", ui->spinBox_rangeDown->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "threshold", ui->spinBox_threshold->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "window_len", ui->spinBox_domainSize->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "matches_reqd", ui->spinBox_matchesReqd->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "direction", 0);
        kpers_set_value(INI_GROUP_ANTI_TEAR, "enabled", ui->groupBox_antiTearingEnabled->isChecked());
    }

    delete ui;

    return;
}

bool AntiTearDialog::is_anti_tear_enabled(void)
{
    return bool(ui->groupBox_antiTearingEnabled->isChecked());
}


void AntiTearDialog::toggle_anti_tear(void)
{
    ui->groupBox_antiTearingEnabled->setChecked(!ui->groupBox_antiTearingEnabled->isChecked());

    return;
}
