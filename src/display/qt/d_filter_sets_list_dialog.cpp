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

    load_sets_from_file(kcom_filters_file_name());

    resize(kpers_value_of("filtering", INI_GROUP_GEOMETRY, size()).toSize());

    create_menu_bar();

    return;
}

FilterSetsListDialog::~FilterSetsListDialog()
{
    kpers_set_value("filtering", INI_GROUP_GEOMETRY, size());

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
    item->setText(3, filterSet.description);

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
                                                    "Filter set files (*.vcsf);;"
                                                    "All files(*.*)");

    if (filename.isNull()) return false;

    load_sets_from_file(filename);

    return true;
}

// Load previously-saved filter sets from the given file.
//
/// TODO. Needs cleanup.
bool FilterSetsListDialog::load_sets_from_file(QString filename)
{
    std::vector<filter_set_s*> setsFromDisk;

    if (filename.isEmpty())
    {
        INFO(("No filter set file defined, skipping."));
        return true;
    }

    QList<QStringList> rowData = csv_parse_c(filename).contents();
    if (rowData.isEmpty())
    {
        goto fail;
    }

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int row = 0; row < rowData.count();)
    {
        filter_set_s *set = new filter_set_s;

        if ((rowData[row].count() != 5) ||
            (rowData[row].at(0) != "inout"))
        {
            NBENE(("Expected a 5-parameter 'inout' statement to begin a filter set block."));
            goto fail;
        }
        set->inRes.w = rowData[row].at(1).toUInt();
        set->inRes.h = rowData[row].at(2).toUInt();
        set->outRes.w = rowData[row].at(3).toUInt();
        set->outRes.h = rowData[row].at(4).toUInt();

        set->activation = 0;
        if (set->inRes.w == 0 && set->inRes.h == 0 &&
            set->outRes.w == 0 && set->outRes.h == 0)
        {
            set->activation |= filter_set_s::activation_e::all;
        }
        else
        {
            if (set->inRes.w != 0 && set->inRes.h != 0) set->activation |= filter_set_s::activation_e::in;
            if (set->outRes.w != 0 && set->outRes.h != 0) set->activation |= filter_set_s::activation_e::out;
        }

        #define verify_first_element_on_row_is(name) if (rowData[row].at(0) != name)\
                                                     {\
                                                        NBENE(("Error while loading the filter set file: expected '%s' but got '%s'.",\
                                                               name, rowData[row].at(0).toStdString().c_str()));\
                                                        goto fail;\
                                                     }

        row++;
        if (rowData[row].at(0) != "enabled") // Legacy support, 'description' was pushed in front of 'enabled' in later revisions.
        {
            verify_first_element_on_row_is("description");
            set->description = rowData[row].at(1);

            row++;
        }
        else
        {
            set->description = "";
        }

        verify_first_element_on_row_is("enabled");
        set->isEnabled = rowData[row].at(1).toInt();

        row++;
        verify_first_element_on_row_is("scaler");
        set->scaler = ks_scaler_for_name_string(rowData[row].at(1));

        row++;
        verify_first_element_on_row_is("preFilters");
        const uint numPreFilters = rowData[row].at(1).toUInt();
        for (uint i = 0; i < numPreFilters; i++)
        {
            row++;
            verify_first_element_on_row_is("pre");

            filter_s filter;

            filter.name = rowData[row].at(1);

            const uint numParams = rowData[row].at(2).toUInt();
            if (numPreFilters >= FILTER_DATA_LENGTH)
            {
                NBENE(("Too many parameters specified for a filter."));
                goto fail;
            }

            for (uint p = 0; p < numParams; p++)
            {
                uint datum = rowData[row].at(3 + p).toInt();
                if (datum > 255)
                {
                    NBENE(("A filter parameter had a value outside of the range allowed (0..255)."));
                    goto fail;
                }
                filter.data[p] = datum;
            }

            set->preFilters.push_back(filter);
        }

        /// TODO. Code duplication.
        row++;
        verify_first_element_on_row_is("postFilters");
        const uint numPostFilters = rowData[row].at(1).toUInt();
        for (uint i = 0; i < numPostFilters; i++)
        {
            row++;
            verify_first_element_on_row_is("post");

            filter_s filter;

            filter.name = rowData[row].at(1);

            const uint numParams = rowData[row].at(2).toUInt();
            if (numPreFilters >= FILTER_DATA_LENGTH)
            {
                NBENE(("Too many parameters specified for a filter."));
                goto fail;
            }

            for (uint p = 0; p < numParams; p++)
            {
                uint datum = rowData[row].at(3 + p).toInt();
                if (datum > 255)
                {
                    NBENE(("A filter parameter had a value outside of the range allowed (0..255)."));
                    goto fail;
                }
                filter.data[p] = datum;
            }

            set->postFilters.push_back(filter);
        }

        #undef verify_first_element_on_row_is

        row++;

        setsFromDisk.push_back(set);
    }

    // Add the filter sets we just loaded to the filterer and the GUI.
    {
        kf_clear_filters();

        for (auto *const set: setsFromDisk)
        {
            kf_add_filter_set(set);
        }

        repopulate_filter_sets_list();
    }

    ui->label_sourceFilename->setText(QFileInfo(filename).fileName());

    INFO(("Loaded %u filter set(s) from disk.", setsFromDisk.size()));
    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter "
                                   "sets. No data was loaded.\n\nMore "
                                   "information about the error may be found in the terminal.");
    return false;
}

bool FilterSetsListDialog::save_sets_to_file(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Save filter sets as...", "",
                                                    "Filter set files (*.vcsf);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return false;
    }

    if (QFileInfo(filename).suffix() != "vcsf") filename.append(".vcsf");

    const QString tempFilename = filename + ".tmp"; // Use a temporary file at first, until we're reasonably sure there were no errors while saving.
    QFile file(tempFilename);
    QTextStream f(&file);
    const std::vector<filter_set_s*> &filterSets = kf_filter_sets();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        NBENE(("Unable to open the filter set file for saving."));
        goto fail;
    }

    for (const auto *set: filterSets)
    {
        // Save the resolutions.
        {
            // Encode the filter set's activation in the resolution values, where 0
            // means the set activates for all resolutions.
            resolution_s inRes = set->inRes, outRes = set->outRes;
            if (set->activation & filter_set_s::activation_e::all)
            {
                inRes = {0, 0};
                outRes = {0, 0};
            }
            else
            {
                if (!(set->activation & filter_set_s::activation_e::in)) inRes = {0, 0};
                if (!(set->activation & filter_set_s::activation_e::out)) outRes = {0, 0};
            }

            f << "inout,"
              << inRes.w << ',' << inRes.h << ','
              << outRes.w << ',' << outRes.h << '\n';
        }

        f << "description,{" << set->description << "}\n";
        f << "enabled," << set->isEnabled << '\n';
        f << "scaler,{" << set->scaler->name << "}\n";

        // Save the filters.
        auto save_filter_data = [&](std::vector<filter_s> filters, const QString &filterType)
        {
            for (auto &filter: filters)
            {
                f << filterType << ",{" << filter.name << "}," << FILTER_DATA_LENGTH;
                for (uint q = 0; q < FILTER_DATA_LENGTH; q++)
                {
                    f << ',' << (u8)filter.data[q];
                }
                f << '\n';
            }
        };
        f << "preFilters," << set->preFilters.size() << '\n';
        save_filter_data(set->preFilters, "pre");
        f << "postFilters," << set->postFilters.size() << '\n';
        save_filter_data(set->postFilters, "post");

        f << '\n';
    }

    file.close();

    // Replace the existing save file with our new data.
    if (QFile(filename).exists())
    {
        if (!QFile(filename).remove())
        {
            NBENE(("Failed to remove old filter set file."));
            goto fail;
        }
    }
    if (!QFile(tempFilename).rename(filename))
    {
        NBENE(("Failed to write filter sets to file."));
        goto fail;
    }

    ui->label_sourceFilename->setText(QFileInfo(filename).fileName());

    INFO(("Saved %u filter set(s) to disk.", filterSets.size()));
    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the filter "
                                   "sets for saving. No data was saved. \n\nMore "
                                   "information about the error may be found in the terminal.");
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
