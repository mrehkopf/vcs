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
#include "../../common/persistent_settings.h"
#include "ui_d_anti_tear_dialog.h"
#include "d_anti_tear_dialog.h"
#include "../../filter/anti_tear.h"
#include "../display.h"
#include "../../common/globals.h"
#include "../../common/csv.h"

AntiTearDialog::AntiTearDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AntiTearDialog)
{
    const anti_tear_options_s a = kat_default_settings();

    ui->setupUi(this);

    setWindowTitle("VCS - Anti-Tear");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->spinBox_rangeUp->setValue(kpers_value_of("range_up", INI_GROUP_ANTI_TEAR, a.rangeUp).toInt());
    ui->spinBox_rangeDown->setValue(kpers_value_of("range_down", INI_GROUP_ANTI_TEAR, a.rangeDown).toInt());
    ui->spinBox_threshold->setValue(kpers_value_of("threshold", INI_GROUP_ANTI_TEAR, a.threshold).toInt());
    ui->spinBox_matchedReqd->setValue(kpers_value_of("matches_reqd", INI_GROUP_ANTI_TEAR, a.matchesReqd).toInt());
    ui->spinBox_domainSize->setValue(kpers_value_of("window_len", INI_GROUP_ANTI_TEAR, a.windowLen).toInt());

    resize(kpers_value_of("anti_tear", INI_GROUP_GEOMETRY, size()).toSize());

    create_menu_bar();

    return;
}

AntiTearDialog::~AntiTearDialog()
{
    kpers_set_value("anti_tear", INI_GROUP_GEOMETRY, ui->spinBox_rangeUp->value());

    kpers_set_value("range_up", INI_GROUP_ANTI_TEAR, ui->spinBox_rangeUp->value());
    kpers_set_value("range_down", INI_GROUP_ANTI_TEAR, ui->spinBox_rangeDown->value());
    kpers_set_value("threshold", INI_GROUP_ANTI_TEAR, ui->spinBox_threshold->value());
    kpers_set_value("window_len", INI_GROUP_ANTI_TEAR, ui->spinBox_domainSize->value());
    kpers_set_value("matches_reqd", INI_GROUP_ANTI_TEAR, ui->spinBox_matchedReqd->value());
    kpers_set_value("direction", INI_GROUP_ANTI_TEAR, 0);

    delete ui;

    return;
}

void AntiTearDialog::create_menu_bar(void)
{
    k_assert((this->menubar == nullptr), "Not allowed to create the anti-tear dialog menu bar more than once.");

    this->menubar = new QMenuBar(this);

    // File...
    {
        QMenu *fileMenu = new QMenu("File", this);

        fileMenu->addAction("Load settings...", this, SLOT(load_settings()));
        fileMenu->addSeparator();
        fileMenu->addAction("Save settings as...", this, SLOT(save_settings()));
        fileMenu->addSeparator();
        fileMenu->addAction("Restore defaults", this, SLOT(restore_default_settings()));

        menubar->addMenu(fileMenu);
    }

    // Help...
    {
        QMenu *fileMenu = new QMenu("Help", this);

        fileMenu->addAction("About...");
        fileMenu->actions().at(0)->setEnabled(false); /// FIXME: Add proper help stuff.

        menubar->addMenu(fileMenu);
    }

    this->layout()->setMenuBar(menubar);

    return;
}

void AntiTearDialog::save_settings(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save anti-tearing settings as...", "",
                                                    "Anti-tear parameters (*.vcst);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    if (QFileInfo(filename).suffix() != "vcst") filename.append(".vcst");

    const QString tempFilename = filename + ".tmp";   // Use a temporary file at first, until we're reasonably sure there were no errors while saving.
    QFile file(tempFilename);
    QTextStream f(&file);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        NBENE(("Unable to open the anti-tear parameter file for saving."));
        goto fail;
    }

    f << "rangeUp," << ui->spinBox_rangeUp->value() << '\n'
      << "rangeDown," << ui->spinBox_rangeDown->value() << '\n'
      << "threshold," << ui->spinBox_threshold->value() << '\n'
      << "windowLen," << ui->spinBox_domainSize->value() << '\n'
      << "matchesReqd," << ui->spinBox_matchedReqd->value() << '\n'
      << "stepSize," << ui->spinBox_stepSize->value() << '\n'
      << "direction,0\n";
    if (file.error() != QFileDevice::NoError)
    {
        NBENE(("Failed to write mode params to file."));
        goto fail;
    }

    file.close();

    // Replace the existing save file with our new data.
    if (QFile(filename).exists())
    {
        if (!QFile(filename).remove())
        {
            NBENE(("Failed to remove old anti-tear params file."));
            goto fail;
        }
    }
    if (!QFile(tempFilename).rename(filename))
    {
        NBENE(("Failed to write anti-tear params to file."));
        goto fail;
    }

    INFO(("Saved anti-tear params to disk."));
    return;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the anti-tear "
                                   "parameters for saving. No data was saved. \n\nMore "
                                   "information about the error may be found in the terminal.");
    return;
}

void AntiTearDialog::load_settings(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load anti-tearing settings from...", "",
                                                    "Anti-tear parameters (*.vcst);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    anti_tear_options_s a = {0};

    QList<QStringList> dataRows = csv_parse_c(filename).contents();
    if (dataRows.isEmpty())
    {
        goto fail;
    }

    for (const auto &row: dataRows)
    {
        if (row.size() != 2)    // Expect two elements per row: a parameter, and its value.
        {
            NBENE(("Unexpected number of row values for '%s'.", row.at(0).toLatin1().constData()));
            goto fail;
        }

        const QString param = row.at(0);
        if (param == "rangeUp") a.rangeUp = row.at(1).toUInt();
        else if (param == "rangeDown") a.rangeDown = row.at(1).toUInt();
        else if (param == "threshold") a.threshold = row.at(1).toDouble();
        else if (param == "windowLen") a.windowLen = row.at(1).toUInt();
        else if (param == "matchesReqd") a.matchesReqd = row.at(1).toUInt();
        else if (param == "stepSize") a.stepSize = row.at(1).toUInt();
        else if (param == "direction") /*do nothing*/;
        else goto fail; // Unknown parameter.
    }

    // Update the GUI elements on the new values.
    kat_set_buffer_updates_disabled(true);
    ui->spinBox_rangeUp->setValue(a.rangeUp);
    ui->spinBox_rangeDown->setValue(a.rangeDown);
    ui->spinBox_threshold->setValue(a.threshold);
    ui->spinBox_matchedReqd->setValue(a.matchesReqd);
    ui->spinBox_domainSize->setValue(a.windowLen);
    ui->spinBox_stepSize->setValue(a.stepSize);
    kat_set_buffer_updates_disabled(false);

    INFO(("Loaded anti-tear params from disk."));
    return;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the anti-tear "
                                   "parameter file. No data was loaded.\n\nMore "
                                   "information about the error may be found in the terminal.");
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
    ui->spinBox_matchedReqd->setValue(a.matchesReqd);
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

void AntiTearDialog::on_spinBox_matchedReqd_valueChanged(int)
{
    kat_set_matches_required(ui->spinBox_matchedReqd->value());

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
