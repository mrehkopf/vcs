/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A QTreeWidget for displaying a list of filters. The user can add and remove
 * filters to and from the list. Used in VCS's filter set dialog.
 *
 */

#include <QGuiApplication>
#include <QTreeWidget>
#include <vector>
#include "display/qt/subclasses/QTreeWidget_filter_list.h"
#include "display/qt/dialogs/filter_dialogs.h"
#include "display/qt/utility.h"
#include "filter/filter.h"
#include "common/memory.h"

FilterListTreeWidget::FilterListTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    connect(this, &QTreeWidget::itemClicked,
            this, [](QTreeWidgetItem *item, int column)
    {
        // Clicking on an item while holding Ctrl removes the item from the list.
        if (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier)
        {
            delete item;
        }

        (void)column;

        return;
    });

    connect(this, &QTreeWidget::itemDoubleClicked,
            this, [this](QTreeWidgetItem *item)
    {
        // Pop up a dialog letting the user set the filter's parameters.
        kf_filter_dialog_for_name(item->text(0).toStdString())->poll_user_for_params(filterData[item], this);

        return;
    });

    connect(this, &QTreeWidget::itemChanged,
            this, [this](QTreeWidgetItem *item)
    {
        // Assume that this signal being emitted means a new item (filter) has
        // been dropped on the list; so initialize a new filter entry accordingly.
        filterData[item] = (u8*)kmem_allocate(FILTER_DATA_LENGTH, "Post-filter data");
        kf_filter_dialog_for_name(item->text(0).toStdString())->insert_default_params(filterData[item]);

        // By default, tree widget items will adopt drops as their children, but
        // we want them to not do so, and instead for all drops to be added as
        // new top-level items, so disallow drops into items.
        { block_widget_signals_c b(this);
            item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        }
    });

    return;
}

FilterListTreeWidget::~FilterListTreeWidget()
{
    for (auto item: filterData)
    {
        kmem_release((void**)&item.second);
    }

    return;
}

// Returns a list of the filters corresponding to the items in this list.
std::vector<filter_s> FilterListTreeWidget::filters(void)
{
    std::vector<filter_s> filterList;

    const auto items = this->findItems("*", Qt::MatchWildcard);
    for (int i = 0; i < items.count(); i++)
    {
        filter_s f;

        f.name = items.at(0)->text(0).toStdString();
        if (!kf_named_filter_exists(f.name))
        {
            NBENE(("Was asked to access a filter that doesn't exist. Ignoring this."));
            continue;
        }

        memcpy(f.data, filterData[items.at(0)], FILTER_DATA_LENGTH);

        filterList.push_back(f);
    }

    return filterList;
}

// Convert the given filters into list items, and assign them as this list's items.
void FilterListTreeWidget::set_filters(const std::vector<filter_s> &filters)
{
    this->clear();

    for (auto &filter: filters)
    {
        block_widget_signals_c b(this);

        QTreeWidgetItem *item = new QTreeWidgetItem(this);

        item->setText(0, QString::fromStdString(filter.name));

        filterData[item] = (u8*)kmem_allocate(FILTER_DATA_LENGTH, "Filter set data");
        memcpy(filterData[item], filter.data, FILTER_DATA_LENGTH);

        this->addTopLevelItem(item);
    }

    return;
}
