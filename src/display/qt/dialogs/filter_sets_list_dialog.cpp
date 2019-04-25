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
#include "display/qt/dialogs/filter_sets_list_dialog.h"
#include "display/qt/dialogs/filter_set_dialog.h"
#include "display/qt/dialogs/filter_dialogs.h"
#include "display/qt/persistent_settings.h"
#include "common/command_line.h"
#include "display/qt/utility.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "filter/filter.h"
#include "scaler/scaler.h"
#include "common/disk.h"
#include "common/csv.h"
#include "ui_filter_sets_list_dialog.h"

// The index of the column in the filter sets tree widget that holds the set's
// enabled/disabled status.
static const uint CHECKBOX_COLUMN_IDX = 0;

// The index of the column in the filter sets tree widget that holds the set's
// description string.
static const uint DESCRIPTION_COLUMN_IDX = 3;

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

    update_selection_sensitive_controls();

    return;
}

FilterSetsListDialog::~FilterSetsListDialog()
{
    kpers_set_value(INI_GROUP_GEOMETRY, "filtering", size());

    delete ui;
    ui = nullptr;

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

    // We'll allow the user to edit the 'Description' column.
    item->setFlags(item->flags() | Qt::ItemIsEditable);

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

void FilterSetsListDialog::update_filter_set_idx(void)
{
    if (!this->filteringIsEnabled) return;

    QString idxStr = "";
    const int idx = kf_current_filter_set_idx();

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
    if (filename.isEmpty())
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

// Enables/disables controls whose such state depends on the current selection
// of items.
//
void FilterSetsListDialog::update_selection_sensitive_controls(void)
{
    const auto selectedItems = ui->treeWidget_setList->selectedItems();
    ui->pushButton_up->setDisabled(selectedItems.isEmpty());
    ui->pushButton_down->setDisabled(selectedItems.isEmpty());
    ui->pushButton_remove->setDisabled(selectedItems.isEmpty());
    ui->pushButton_editSelected->setDisabled(selectedItems.isEmpty());

    return;
}

// Repopulates the filter sets list based on which filter sets are available to the filterer.
// If given an index, will set the item at that index (row) as the current item.
//
void FilterSetsListDialog::repopulate_filter_sets_list(int newIdx)
{
    ui->treeWidget_setList->clear();

    const auto filterSets = kf_filter_sets();
    for (uint i = 0; i < filterSets.size(); i++)
    {
        auto listItem = make_filter_set_tree_item(*filterSets.at(i), i);
        ui->treeWidget_setList->addTopLevelItem(listItem);
    }

    if (newIdx != -1) // Assumes newIdx defaults to -1 if no value is specified.
    {
        newIdx = std::max(0, std::min(newIdx, int(filterSets.size() - 1)));
        auto targetItem = ui->treeWidget_setList->findItems("*", Qt::MatchWildcard).at(newIdx);
        ui->treeWidget_setList->setCurrentItem(targetItem);
    }
    else
    {
        // Set to have no selection.
        ui->treeWidget_setList->setCurrentIndex(QModelIndex());
    }

    update_selection_sensitive_controls();

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

// Trigger the filter set edit dialog on the filter set corresponding to the
// given list item. Once the user has closed the dialog, its parameters will
// be assigned to that filter set.
//
void FilterSetsListDialog::edit_set(const QTreeWidgetItem *const item)
{
    // Find out which filter set the item corresponds to.
    const int filterSetIdx = item->data(0, Qt::UserRole).value<uint>();
    filter_set_s *const filterSet = kf_filter_sets().at(filterSetIdx);

    FilterSetDialog *setDialog = new FilterSetDialog(filterSet, this);
    setDialog->open();

    connect(setDialog, &FilterSetDialog::accepted,
                 this, [this]
    {
        repopulate_filter_sets_list();
        flag_unsaved_changes();
    });

    return;
}

// On double-clicking an item in the set list, open the filter set dialog to let
// the user modify the set's parameters.
//
void FilterSetsListDialog::on_treeWidget_setList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    switch (column)
    {
        // The set's enabled/disabled checkbox could potentially generate
        // accidental double-clicks, so ignore double-clicks on it altogether.
        case CHECKBOX_COLUMN_IDX:
        {
            return;
        }

        // Let the user edit the 'Description' column's text. An itemChanged
        // signal will be emitted when the user finishes this edit, to which
        // we'll react elsewhere by updating the set's description with the
        // user's text.
        case DESCRIPTION_COLUMN_IDX:
        {
            ui->treeWidget_setList->editItem(item, column);

            return;
        }

        // Open a dialog box to let the user customize the properties of the
        // corresponding filter set.
        default:
        {
            edit_set(item);

            return;
        }
    }

    (void)column;
}

void FilterSetsListDialog::on_treeWidget_setList_itemChanged(QTreeWidgetItem *item, int column)
{
    // Find out which filter set this item corresponds to.
    const int filterSetIdx = item->data(0, Qt::UserRole).value<uint>();
    filter_set_s *const filterSet = kf_filter_sets().at(filterSetIdx);

    switch (column)
    {
        // Enable or disable the filter set.
        case CHECKBOX_COLUMN_IDX:
        {
            kf_set_filter_set_enabled(filterSetIdx, (item->checkState(CHECKBOX_COLUMN_IDX) == Qt::Checked));

            return;
        }

        // The user finished editing the set's description, so update the set
        // with that information.
        case DESCRIPTION_COLUMN_IDX:
        {
            filterSet->description = item->text(DESCRIPTION_COLUMN_IDX).toStdString();

            return;
        }

        // Ignore all other changes.
        default: return;
    }
}

void FilterSetsListDialog::on_pushButton_remove_clicked()
{
    k_assert((ui->treeWidget_setList->selectionMode() == QAbstractItemView::SingleSelection),
             "Expected the tree widget's selection mode to be for single items.");

    const auto selectedItems = ui->treeWidget_setList->selectedItems();
    if (selectedItems.empty()) return;

    const int filterSetIdx = selectedItems.at(0)->data(0, Qt::UserRole).value<uint>();
    kf_remove_filter_set(filterSetIdx);

    repopulate_filter_sets_list(-1);

    flag_unsaved_changes();

    return;
}

void FilterSetsListDialog::on_pushButton_add_clicked()
{
    add_new_set();

    return;
}

// Moves the given item vertically in the list by one position in the given
// direction.
void FilterSetsListDialog::move_item_vertically(const QTreeWidgetItem *const item,
                                                const int direction)
{
    const int filterSetIdx = item->data(0, Qt::UserRole).value<uint>();

    switch (direction)
    {
        case -1: kf_filter_set_swap_upward(filterSetIdx); break;
        case  1: kf_filter_set_swap_downward(filterSetIdx); break;
        default: k_assert(0, "Unknown direction for vertical move in filter set list."); break;
    }

    const uint newIdx = std::max(0, std::min((filterSetIdx + direction), int(kf_filter_sets().size() - 1)));
    repopulate_filter_sets_list(newIdx);

    flag_unsaved_changes();

    return;
}

void FilterSetsListDialog::on_pushButton_up_clicked()
{
    k_assert((ui->treeWidget_setList->selectionMode() == QAbstractItemView::SingleSelection),
             "Expected the tree widget's selection mode to be for single items.");

    const auto selectedItems = ui->treeWidget_setList->selectedItems();
    if (selectedItems.empty()) return;

    move_item_vertically(selectedItems.at(0), -1);

    return;
}

void FilterSetsListDialog::on_pushButton_down_clicked()
{
    k_assert((ui->treeWidget_setList->selectionMode() == QAbstractItemView::SingleSelection),
             "Expected the tree widget's selection mode to be for single items.");

    const auto selectedItems = ui->treeWidget_setList->selectedItems();
    if (selectedItems.empty()) return;

    move_item_vertically(selectedItems.at(0), 1);

    return;
}

void FilterSetsListDialog::on_pushButton_editSelected_clicked()
{
    k_assert((ui->treeWidget_setList->selectionMode() == QAbstractItemView::SingleSelection),
             "Expected the tree widget's selection mode to be for single items.");

    const auto selectedItems = ui->treeWidget_setList->selectedItems();
    if (selectedItems.empty()) return;

    edit_set(selectedItems.at(0));

    return;
}

void FilterSetsListDialog::on_treeWidget_setList_itemSelectionChanged()
{
    update_selection_sensitive_controls();

    return;
}
