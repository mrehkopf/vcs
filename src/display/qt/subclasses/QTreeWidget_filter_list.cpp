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
#include <cstring>
#include <vector>
#include "display/qt/subclasses/QTreeWidget_filter_list.h"
#include "display/qt/dialogs/filter_dialogs.h"
#include "display/qt/utility.h"
#include "filter/filter.h"
#include "filter/filter_legacy.h"
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
        const auto uuid = kf_filter_uuid_for_name(item->text(0).toStdString());
        kf_filter_dialog_for_uuid(uuid)->poll_user_for_params(filterData[item], this);

        return;
    });

    connect(this, &QTreeWidget::itemChanged,
            this, [this](QTreeWidgetItem *item)
    {
        // Assume that this signal being emitted means a new item (filter) has
        // been dropped on the list; so initialize a new filter entry accordingly.
        filterData[item] = (u8*)kmem_allocate(FILTER_PARAMETER_ARRAY_LENGTH, "Post-filter data");
        const auto uuid = kf_filter_uuid_for_name(item->text(0).toStdString());
        kf_filter_dialog_for_uuid(uuid)->insert_default_params(filterData[item]);

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
std::vector<legacy14_filter_s> FilterListTreeWidget::filters(void)
{
    std::vector<legacy14_filter_s> filterList;

    const auto items = this->findItems("*", Qt::MatchWildcard);
    for (auto *item: items)
    {
        legacy14_filter_s f;

        f.name = item->text(0).toStdString();
        f.uuid = kf_filter_uuid_for_name(f.name);

        if (!kf_filter_exists(f.uuid))
        {
            NBENE(("Was asked to access a filter that doesn't exist. Ignoring this."));
            continue;
        }

        memcpy(f.data, filterData[item], FILTER_PARAMETER_ARRAY_LENGTH);

        filterList.push_back(f);
    }

    return filterList;
}

// Convert the given filters into list items, and assign them as this list's items.
void FilterListTreeWidget::set_filters(const std::vector<legacy14_filter_s> &filters)
{
    this->clear();

    for (auto &filter: filters)
    {
        block_widget_signals_c b(this);

        QTreeWidgetItem *item = new QTreeWidgetItem(this);

        item->setText(0, QString::fromStdString(filter.name));

        filterData[item] = (u8*)kmem_allocate(FILTER_PARAMETER_ARRAY_LENGTH, "Filter set data");
        memcpy(filterData[item], filter.data, FILTER_PARAMETER_ARRAY_LENGTH);

        this->addTopLevelItem(item);
    }

    return;
}
