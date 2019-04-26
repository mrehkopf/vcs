/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter sets dialog
 *
 * A GUI dialog for managing filter sets - collections of filters to be applied
 * to incoming captured frames.
 *
 */

#include <QListWidgetItem>
#include <QFileDialog>
#include <QMenuBar>
#include <QDebug>
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

    this->setWindowTitle("VCS - Filter Sets");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Set the GUI controls to their proper initial values.
    {
        // Create the dialog's menu bar.
        {
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
        }

        // Restore persistent settings.
        {
            this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "filtering", size()).toSize());
        }

        // If the user supplied a filter sets file on the command-line when starting
        // the program, load it in.
        kdisk_load_filter_sets(kcom_filters_file_name());

        update_selection_sensitive_controls();
    }

    // Connect GUI controls to consequences for operating them.
    {
        // Returns a pointer to the currently-selected item in the filter sets
        // list, or nullptr if none is selected.
        auto selected_item = [this]()->QTreeWidgetItem*
        {
            k_assert((ui->treeWidget_setList->selectionMode() == QAbstractItemView::SingleSelection),
                     "Expected the tree widget's selection mode to be for single items.");

            const auto selectedItems = ui->treeWidget_setList->selectedItems();

            if (selectedItems.empty())
            {
                return nullptr;
            }
            else return ui->treeWidget_setList->selectedItems().at(0);
        };

        // Moves the currently-selected item in the filter sets list vertically
        // by one position in the given direction.
        auto move_selected_item_vertically = [=](const int direction)
        {
            if (!selected_item()) return;

            const int filterSetIdx = selected_item()->data(0, Qt::UserRole).value<uint>();

            switch (direction)
            {
                case -1: kf_filter_set_swap_upward(filterSetIdx); break;
                case  1: kf_filter_set_swap_downward(filterSetIdx); break;
                default: k_assert(0, "Unknown direction for vertical move in filter set list."); break;
            }

            const uint newIdx = std::max(0, std::min((filterSetIdx + direction), int(kf_filter_sets().size() - 1)));
            repopulate_filter_sets_list(newIdx);
            flag_unsaved_changes();
        };

        connect(ui->pushButton_up, &QPushButton::clicked, this, [=]
        {
            move_selected_item_vertically(-1);
        });

        connect(ui->pushButton_down, &QPushButton::clicked, this, [=]
        {
            move_selected_item_vertically(1);
        });

        connect(ui->pushButton_editSelected, &QPushButton::clicked, this, [=]
        {
            edit_item(selected_item());
        });

        connect(ui->pushButton_removeSelected, &QPushButton::clicked, this, [=]
        {
            if (!selected_item()) return;

            const int filterSetIdx = selected_item()->data(0, Qt::UserRole).value<uint>();
            kf_remove_filter_set(filterSetIdx);

            repopulate_filter_sets_list(-1);
            flag_unsaved_changes();
        });

        // Lets the user create a new filter set to add to the list, by spawning a dialog
        // in which the user can define the attributes of said set. Once the dialog is
        // closed, the new set will be added to the sets list.
        connect(ui->pushButton_add, &QPushButton::clicked, this, [=]
        {
            filter_set_s *newSet = new filter_set_s;

            FilterSetDialog *setDialog = new FilterSetDialog(newSet, this, false);
            setDialog->open(); /// FIXME: Can't use exec(), otherwise event loop issues come up with mouse drag inside the dialog.

            connect(setDialog, &FilterSetDialog::accepted, this, [=]
            {
                kf_add_filter_set(newSet);
                this->repopulate_filter_sets_list();
                this->flag_unsaved_changes();
            });

            connect(setDialog, &FilterSetDialog::rejected, this, [=]
            {
                delete newSet;
            });
        });

        connect(ui->treeWidget_setList, &QTreeWidget::itemSelectionChanged, this, [this]
        {
            update_selection_sensitive_controls();
        });

        // On double-clicking an item in the set list, open the filter set dialog to let
        // the user modify the set's parameters.
        connect(ui->treeWidget_setList, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int column)
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
                    edit_item(item);
                    return;
                }
            }
        });

        connect(ui->treeWidget_setList, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column)
        {
            flag_unsaved_changes();

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
        });
    }

    return;
}

FilterSetsListDialog::~FilterSetsListDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "filtering", size());
    }

    delete ui;
    ui = nullptr;

    return;
}

// Called to inform this dialog whether filtering is enabled (in the control
// panel). This will be (or should be) called each time the state is toggled,
// to keep the dialog up-to-date on that.
void FilterSetsListDialog::signal_filtering_enabled(const bool state)
{
    filteringIsEnabled = state;

    update_filter_set_idx();

    return;
}

// Called to tell this dialog to update its list of filter sets - for instance,
// because the selection of filter sets has changed.
void FilterSetsListDialog::update_filter_sets_list(void)
{
    this->repopulate_filter_sets_list();

    return;
}

// Called to tell this dialog to update its information about which filter set
// is currently active.
void FilterSetsListDialog::update_filter_set_idx(void)
{
    if (!this->filteringIsEnabled ||
        (kf_current_filter_set_idx() == -1))
    {
        goto no_active_set;
    }

    ui->label_activeSet->setText(QString::number(kf_current_filter_set_idx() + 1));
    ui->label_activeSet->setEnabled(true);
    return;

    no_active_set:
    ui->label_activeSet->setText("n/a");
    ui->label_activeSet->setEnabled(false);
    return;
}

bool FilterSetsListDialog::load_sets_from_file(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Load filter sets from...", "",
                                                    "Filter set files (*.vcs-filtersets *.vcsf );;"
                                                    "All files(*.*)");

    if (filename.isEmpty()) return false;

    if (kdisk_load_filter_sets(filename.toStdString()))
    {
        remove_unsaved_changes_flag();

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

    if (filename.isEmpty()) return false;

    if (!QStringList({"vcs-filtersets", "vcsf"}).contains(QFileInfo(filename).suffix()))
    {
        filename.append(".vcs-filtersets");
    }

    if (kdisk_save_filter_sets(kf_filter_sets(), filename))
    {
        remove_unsaved_changes_flag();

        ui->label_sourceFilename->setText(QFileInfo(filename).fileName());

        return true;
    }

    return false;
}

// Activates a user-facing visual indication that there are unsaved changes in
// the dialog.
void FilterSetsListDialog::flag_unsaved_changes(void)
{
    if (!this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().prepend("*"));
    }

    return;
}

// Deactivates the user-facing visual indication that there are unsaved changes
// in the dialog.
void FilterSetsListDialog::remove_unsaved_changes_flag(void)
{
    if (this->windowTitle().startsWith("*"))
    {
        this->setWindowTitle(this->windowTitle().remove(0, 1));
    }

    return;
}

// Enables/disables controls whose such state depends on the current selection
// of items.
void FilterSetsListDialog::update_selection_sensitive_controls(void)
{
    const bool noSelection = ui->treeWidget_setList->selectedItems().isEmpty();

    ui->pushButton_up->setDisabled(noSelection);
    ui->pushButton_down->setDisabled(noSelection);
    ui->pushButton_removeSelected->setDisabled(noSelection);
    ui->pushButton_editSelected->setDisabled(noSelection);

    return;
}

// Repopulates the filter sets list based on which filter sets are available to
// the VCS filterer. If given an index, will set the item at that index (row) as
// the currently-selected one.
void FilterSetsListDialog::repopulate_filter_sets_list(int newIdx)
{
    ui->treeWidget_setList->clear();

    auto make_filter_set_tree_item = [](const filter_set_s &filterSet,
                                        const uint filterSetIdx)->QTreeWidgetItem*
    {
        QTreeWidgetItem *item = new QTreeWidgetItem;

        const QString inputRes = (filterSet.activation & filter_set_s::activation_e::in)?
                                 QString("%1 x %2").arg(filterSet.inRes.w).arg(filterSet.inRes.h)
                                 : "*";

        const QString outputRes = (filterSet.activation & filter_set_s::activation_e::out)?
                                  QString("%1 x %2").arg(filterSet.outRes.w).arg(filterSet.outRes.h)
                                  : "*";

        item->setText(0, QString::number(filterSetIdx + 1));
        item->setCheckState(0, filterSet.isEnabled? Qt::Checked : Qt::Unchecked);
        item->setText(1, inputRes);
        item->setText(2, outputRes);
        item->setText(3, QString::fromStdString(filterSet.description));

        // We'll allow the user to edit the 'Description' column.
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        item->setData(0, Qt::UserRole, filterSetIdx);

        return item;
    };

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

// Lets the user edit a particular filter set given in the sets list, by spawning
// a dialog in which the user can customize the attributes of said set.
void FilterSetsListDialog::edit_item(const QTreeWidgetItem *const item)
{
    if (item == nullptr) return;

    // Find out which filter set the list item corresponds to.
    const int filterSetIdx = item->data(0, Qt::UserRole).value<uint>();
    filter_set_s *const filterSet = kf_filter_sets().at(filterSetIdx);

    FilterSetDialog *setDialog = new FilterSetDialog(filterSet, this);
    setDialog->open();

    connect(setDialog, &FilterSetDialog::accepted, this, [=]
    {
        repopulate_filter_sets_list();
        flag_unsaved_changes();
    });

    return;
}
