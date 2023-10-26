/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QHeaderView>
#include "display/qt/widgets/PropertyTable.h"

PropertyTable::PropertyTable(QWidget *parent) : QTableWidget(parent)
{
    this->insertColumn(0);
    this->insertColumn(1);
    this->setHorizontalHeaderItem(0, new QTableWidgetItem("Property"));
    this->setHorizontalHeaderItem(1, new QTableWidgetItem("Value"));

    this->setGridStyle(Qt::NoPen);

    this->horizontalHeader()->hide();
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    this->verticalHeader()->hide();
    this->verticalHeader()->setDefaultSectionSize(25);
    this->verticalHeader()->setMinimumSectionSize(25);

    this->setShowGrid(false);
    this->setAlternatingRowColors(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    return;
}

unsigned PropertyTable::add_property(const QString &name, const QString &tooltip)
{
    const unsigned rowIdx = this->rowCount();

    this->insertRow(rowIdx);
    this->setItem(rowIdx, 0, new QTableWidgetItem(name));
    this->setItem(rowIdx, 1, new QTableWidgetItem("-"));

    this->item(rowIdx, 0)->setToolTip(tooltip);
    this->item(rowIdx, 1)->setToolTip(tooltip);

    return rowIdx;
}

void PropertyTable::reset_property_values(void)
{
    for (int i = 0; i < this->rowCount(); i++)
    {
        this->item(i, 1)->setText("-");
    }

    return;
}

int PropertyTable::find_row_idx(const QString &name)
{
    for (int i = 0; i < this->rowCount(); i++)
    {
        if (this->item(i, 0)->text() == name)
        {
            return i;
        }
    }

    return -1;
}

void PropertyTable::modify_property(const QString &propertyName, const QString &value)
{
    int rowIdx = this->find_row_idx(propertyName);

    if (rowIdx == -1)
    {
        rowIdx = this->add_property(propertyName);
    }

    this->item(rowIdx, 1)->setText(value);

    return;
}
