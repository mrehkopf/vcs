/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear dialog
 *
 * Provides GUI controls for the user to customize the anti-tear engine's
 * parameters.
 *
 */

#include <QMenuBar>
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

                ui->groupBox_parameters->setEnabled(true);
                ui->groupBox_visualization->setEnabled(true);
            });

            connect(this, &AntiTearDialog::anti_tear_disabled, this, [=]
            {
                enable->setChecked(false);
                update_visualization_options();

                ui->groupBox_parameters->setEnabled(false);
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

    // Connect the GUI controls to consequences for changing their values.
    {
        // Wire up the buttons for resetting individual video parameters.
        for (int i = 0; i < ui->groupBox_parameters->layout()->count(); i++)
        {
            QWidget *const button = ui->groupBox_parameters->layout()->itemAt(i)->widget();

            // The button name is expected to identify its video parameter affiliation. E.g.
            // a button called "pushButton_reset_value_horPos" is for resetting the horizontal
            // video position.
            if (!button->objectName().startsWith("pushButton_reset_value_"))
            {
                continue;
            }

            connect((QPushButton*)button, &QPushButton::clicked, this, [this, button]
            {
                const QString paramId = button->objectName().replace("pushButton_reset_value_", "");
                const auto defaults = kat_default_settings();

                kat_set_buffer_updates_disabled(true);

                if (paramId == "scanStart") ui->spinBox_rangeUp->setValue(defaults.rangeUp);
                else if (paramId == "scanEnd") ui->spinBox_rangeDown->setValue(defaults.rangeDown);
                else if (paramId == "threshold") ui->spinBox_threshold->setValue(defaults.threshold);
                else if (paramId == "windowSize") ui->spinBox_domainSize->setValue(defaults.windowLen);
                else if (paramId == "stepSize") ui->spinBox_stepSize->setValue(defaults.stepSize);
                else if (paramId == "matchesReqd") ui->spinBox_matchesReqd->setValue(defaults.matchesReqd);
                else k_assert(0, "Unknown parameter identifier");

                kat_set_buffer_updates_disabled(false);
            });
        }

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

        #undef OVERLOAD_INT

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
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_up", ui->spinBox_rangeUp->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "range_down", ui->spinBox_rangeDown->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "threshold", ui->spinBox_threshold->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "window_len", ui->spinBox_domainSize->value());
        kpers_set_value(INI_GROUP_ANTI_TEAR, "matches_reqd", ui->spinBox_matchesReqd->value());
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
