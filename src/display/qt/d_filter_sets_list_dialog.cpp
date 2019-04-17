/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter sets dialog
 *
 * A GUI dialog for managing filter sets - collections of filters to be applied
 * to incoming captured frames.
 *
 */

#include <QListWidgetItem>
#include <QFormLayout>
#include <QFileDialog>
#include <QSplitter>
#include <QMenuBar>
#include <QDebug>
#include <QFile>
#include "../../display/qt/d_filter_sets_list_dialog.h"
#include "../../display/qt/d_filter_set_dialog.h"
#include "../../common/persistent_settings.h"
#include "../../display/qt/df_filters.h"
#include "../../common/command_line.h"
#include "../../display/qt/d_util.h"
#include "../../capture/capture.h"
#include "../../common/globals.h"
#include "../../filter/filter.h"
#include "../../scaler/scaler.h"
#include "../../common/disk.h"
#include "../../common/csv.h"
#include "../display.h"
#include "ui_d_filter_sets_list_dialog.h"

FilterSetsListDialog::FilterSetsListDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterSetsListDialog)
{
    ui->setupUi(this);

    setWindowTitle("VCS - Filter Sets");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    kdisk_load_filter_sets(kcom_filters_file_name());

    resize(kpers_value_of(INI_GROUP_GEOMETRY, "filtering", size()).toSize());

    create_menu_bar();

    return;
}

FilterSetsListDialog::~FilterSetsListDialog()
{
    kpers_set_value(INI_GROUP_GEOMETRY, "filtering", size());

    delete ui; ui = nullptr;

    return;
}

static QTreeWidgetItem* make_filter_set_tree_item(const filter_set_s &filterSet, const uint setId /*index in filterer's list of filter sets*/)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;

    const QString inputRes = (filterSet.activation & filter_set_s::activation_e::in)?
                             QString("%1 x %2").arg(filterSet.inRes.w).arg(filterSet.inRes.h)
                             : "*";

    const QString outputRes = (filterSet.activation & filter_set_s::activation_e::out)?
                              QString("%1 x %2").arg(filterSet.outRes.w).arg(filterSet.outRes.h)
                              : "*";

    item->setText(0, QString::number(setId + 1));
    item->setCheckState(0, filterSet.isEnabled? Qt::Checked : Qt::Unchecked);
    item->setText(1, inputRes);
    item->setText(2, outputRes);
    item->setText(3, QString::fromStdString(filterSet.description));

    item->setData(0, Qt::UserRole, setId);

    return item;
}

void FilterSetsListDialog::create_menu_bar()
{
    k_assert((this->menubar == nullptr), "Not allowed to create the video/color dialog menu bar more than once.");

    this->menubar = new QMenuBar(this);

    // File...
    {
        QMenu *fileMenu = new QMenu("File", this);

        fileMenu->addAction("Load sets...", this, SLOT(load_sets_from_file()));
        fileMenu->addSeparator();
        fileMenu->addAction("Save sets as...", this, SLOT(save_sets_to_file()));

        menubar->addMenu(fileMenu);
    }

    // Help...
    {
        QMenu *fileMenu = new QMenu("Help", this);

        fileMenu->addAction("About...");
        fileMenu->actions().at(0)->setEnabled(false); /// TODO: Add the actual help.

        menubar->addMenu(fileMenu);
    }

    this->layout()->setMenuBar(menubar);

    return;
}

void FilterSetsListDialog::signal_filtering_enabled(const bool state)
{
    filteringIsEnabled = state;

    if (!state) ui->label_activeSet->setText("none");

    return;
}

void FilterSetsListDialog::update_filter_sets_list(void)
{
    this->repopulate_filter_sets_list();

    return;
}

void FilterSetsListDialog::receive_current_filter_set_idx(const int idx)
{
    if (!this->filteringIsEnabled) return;

    QString idxStr = "";

    if (idx < 0)
    {
        idxStr = "none";
        if (idxStr == ui->label_activeSet->text()) return;
    }
    else
    {
        bool goodConversion = false;
        const int prevIdx = (ui->label_activeSet->text().toInt(&goodConversion) - 1);

        if (goodConversion && (idx == prevIdx)) return;

        idxStr = QString::number(idx + 1);
    }

    ui->label_activeSet->setText(idxStr);

    return;
}

// Used to detect when filter sets' checkboxes are checked.
//
void FilterSetsListDialog::on_treeWidget_setList_itemChanged(QTreeWidgetItem *item)
{
    const int idx = item->data(0, Qt::UserRole).value<uint>();
    if (idx < 0) return;

    const bool enabled = (item->checkState(0) == Qt::Checked)? 1 : 0;

    kf_set_filter_set_enabled(idx, enabled);

    return;
}

bool FilterSetsListDialog::load_sets_from_file(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load filter sets from...", "",
                                                    "Filter set files (*.vcs-filtersets *.vcsf );;"
                                                    "All files(*.*)");

    if (filename.isNull()) return false;

    if (kdisk_load_filter_sets(filename.toStdString()))
    {
        ui->label_sourceFilename->setText(QFileInfo(filename).fileName());
        return true;
    }

    return false;
}

bool FilterSetsListDialog::save_sets_to_file(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save filter sets as...", "",
                                                    "Filter set files (*.vcs-filtersets *.vcsf);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return false;
    }

    if (!QStringList({"vcs-filtersets", "vcsf"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-filtersets");
    }

    if (kdisk_save_filter_sets(kf_filter_sets(), filename))
    {
        ui->label_sourceFilename->setText(QFileInfo(filename).fileName());

        return true;
    }

    return false;
}

// Call this to give the user a visual indication that there are unsaved changes
// in the settings.
//
void FilterSetsListDialog::flag_unsaved_changes(void)
{
    const QString filenameString = ui->label_sourceFilename->text();

    if (!filenameString.startsWith("*"))
    {
        ui->label_sourceFilename->setText("*" + filenameString);
    }

    return;
}

// Repopulates the filter sets list based on which filter sets are available to the filterer.
// if given an index, will set the item at that index (row) as the current item.
//
void FilterSetsListDialog::repopulate_filter_sets_list(const int newIdx)
{
    ui->treeWidget_setList->clear();

    const auto filterSets = kf_filter_sets();
    for (uint i = 0; i < filterSets.size(); i++)
    {
        ui->treeWidget_setList->addTopLevelItem(make_filter_set_tree_item(*filterSets.at(i), i));
    }

    if (newIdx >= 0)
    {
        const auto targetItem = ui->treeWidget_setList->findItems("*", Qt::MatchWildcard).at(newIdx);
        ui->treeWidget_setList->setCurrentItem(targetItem);
    }

    return;
}

void FilterSetsListDialog::add_new_set()
{
    filter_set_s *newSet = new filter_set_s;
    FilterSetDialog *setDialog = new FilterSetDialog(newSet, this, false);
    setDialog->open(); /// FIXME: Can't use exec(), otherwise event loop issues come up with mouse drag inside the dialog.

    connect(setDialog, &FilterSetDialog::accepted,
                 this, [this, newSet]
    {
        kf_add_filter_set(newSet);
        repopulate_filter_sets_list();
        flag_unsaved_changes();
    });
    connect(setDialog, &FilterSetDialog::rejected,
                 this, [this, newSet]
    {
        delete newSet;
    });

    return;
}

// On double-clicking an item in the set list, open the filter set dialog to let
// the user modify the set's parameters.
//
void FilterSetsListDialog::on_treeWidget_setList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    // Assume that the first column holds a toggleable checkbox. This could
    // easily generate accidental double-clicks, so ignore it.
    if (column == 0) return;

    const int idx = item->data(0, Qt::UserRole).value<uint>();

    filter_set_s *const set = kf_filter_sets().at(idx);
    FilterSetDialog *setDialog = new FilterSetDialog(set, this);
    setDialog->open();

    connect(setDialog, &FilterSetDialog::accepted,
                 this, [this]
    {
        repopulate_filter_sets_list();
        flag_unsaved_changes();
    });

    return;

    (void)column;
}

void FilterSetsListDialog::on_pushButton_remove_clicked()
{
    const QTreeWidgetItem *itm = ui->treeWidget_setList->currentItem();
    if (itm == nullptr) return;

    const int idx = itm->data(0, Qt::UserRole).value<uint>();

    kf_remove_filter_set(idx);
    repopulate_filter_sets_list();

    flag_unsaved_changes();

    return;
}

void FilterSetsListDialog::on_pushButton_add_clicked()
{
    add_new_set();

    return;
}

void FilterSetsListDialog::on_pushButton_up_clicked()
{
    const QTreeWidgetItem *itm = ui->treeWidget_setList->currentItem();
    if (itm == nullptr) return;

    const int idx = itm->data(0, Qt::UserRole).value<uint>();
    if (idx <= 0) return;

    kf_filter_set_swap_upward(idx);
    repopulate_filter_sets_list(idx-1);

    flag_unsaved_changes();

    return;
}

void FilterSetsListDialog::on_pushButton_down_clicked()
{
    const QTreeWidgetItem *itm = ui->treeWidget_setList->currentItem();
    if (itm == nullptr) return;

    const int idx = itm->data(0, Qt::UserRole).value<uint>();
    if (idx >= (ui->treeWidget_setList->topLevelItemCount() - 1)) return;

    kf_filter_set_swap_downward(idx);
    repopulate_filter_sets_list(idx+1);

    flag_unsaved_changes();

    return;
}
