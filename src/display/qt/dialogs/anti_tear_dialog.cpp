/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS anti-tear dialog
 *
 * Allows the user GUI controls to the anti-tear engine's parameters
 *
 */

#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QMenuBar>
#include <QFile>
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/persistent_settings.h"
#include "filter/anti_tear.h"
#include "display/display.h"
#include "common/globals.h"
#include "common/csv.h"
#include "ui_anti_tear_dialog.h"

AntiTearDialog::AntiTearDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AntiTearDialog)
{
    ui->setupUi(this);

    setWindowTitle("VCS - Anti-Tearing");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    const anti_tear_options_s a = kat_default_settings();
    ui->spinBox_rangeUp->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "range_up", a.rangeUp).toInt());
    ui->spinBox_rangeDown->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "range_down", a.rangeDown).toInt());
    ui->spinBox_threshold->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "threshold", a.threshold).toInt());
    ui->spinBox_matchesReqd->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "matches_reqd", a.matchesReqd).toInt());
    ui->spinBox_domainSize->setValue(kpers_value_of(INI_GROUP_ANTI_TEAR, "window_len", a.windowLen).toInt());

    resize(kpers_value_of(INI_GROUP_GEOMETRY, "anti_tear", this->size()).toSize());

    return;
}

AntiTearDialog::~AntiTearDialog()
{
    kpers_set_value(INI_GROUP_GEOMETRY, "anti_tear", this->size());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "range_up", ui->spinBox_rangeUp->value());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "range_down", ui->spinBox_rangeDown->value());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "threshold", ui->spinBox_threshold->value());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "window_len", ui->spinBox_domainSize->value());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "matches_reqd", ui->spinBox_matchesReqd->value());
    kpers_set_value(INI_GROUP_ANTI_TEAR, "direction", 0);

    delete ui;

    return;
}

void AntiTearDialog::update_visualization_options()
{
    const bool enabled = ui->groupBox_visualization->isChecked();

    kat_set_visualization(enabled,
                          ui->checkBox_visualizeTear->isChecked(),
                          ui->checkBox_visualizeRange->isChecked());

    return;
}

void AntiTearDialog::restore_default_settings(void)
{
    const anti_tear_options_s a = kat_default_settings();

    kat_set_buffer_updates_disabled(true);
    ui->spinBox_rangeUp->setValue(a.rangeUp);
    ui->spinBox_rangeDown->setValue(a.rangeDown);
    ui->spinBox_threshold->setValue(a.threshold);
    ui->spinBox_matchesReqd->setValue(a.matchesReqd);
    ui->spinBox_domainSize->setValue(a.windowLen);
    ui->spinBox_stepSize->setValue(a.stepSize);
    kat_set_buffer_updates_disabled(false);

    return;
}

void AntiTearDialog::on_spinBox_rangeUp_valueChanged(int)
{
    kat_set_range(ui->spinBox_rangeUp->value(),
                  ui->spinBox_rangeDown->value());

    return;
}

void AntiTearDialog::on_spinBox_rangeDown_valueChanged(int)
{
    kat_set_range(ui->spinBox_rangeUp->value(),
                  ui->spinBox_rangeDown->value());

    return;
}

void AntiTearDialog::on_spinBox_threshold_valueChanged(int)
{
    kat_set_threshold(ui->spinBox_threshold->value());

    return;
}

void AntiTearDialog::on_spinBox_domainSize_valueChanged(int)
{
    kat_set_domain_size(ui->spinBox_domainSize->value());

    return;
}

void AntiTearDialog::on_spinBox_matchesReqd_valueChanged(int)
{
    kat_set_matches_required(ui->spinBox_matchesReqd->value());

    return;
}

void AntiTearDialog::on_spinBox_stepSize_valueChanged(int)
{
    kat_set_step_size(ui->spinBox_stepSize->value());

    return;
}

void AntiTearDialog::on_groupBox_visualization_toggled(bool)
{
    update_visualization_options();

    return;
}

void AntiTearDialog::on_checkBox_visualizeRange_stateChanged(int)
{
    update_visualization_options();

    return;
}

void AntiTearDialog::on_checkBox_visualizeTear_stateChanged(int)
{
    update_visualization_options();

    return;
}

void AntiTearDialog::on_pushButton_resetDefaults_clicked()
{
    const anti_tear_options_s a = kat_default_settings();

    kat_set_buffer_updates_disabled(true);
    ui->spinBox_rangeUp->setValue(a.rangeUp);
    ui->spinBox_rangeDown->setValue(a.rangeDown);
    ui->spinBox_threshold->setValue(a.threshold);
    ui->spinBox_matchesReqd->setValue(a.matchesReqd);
    ui->spinBox_domainSize->setValue(a.windowLen);
    ui->spinBox_stepSize->setValue(a.stepSize);
    kat_set_buffer_updates_disabled(false);

    return;
}
