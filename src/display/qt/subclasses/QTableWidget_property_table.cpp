/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QHeaderView>
#include "display/qt/subclasses/QTableWidget_property_table.h"

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
    this->verticalHeader()->setDefaultSectionSize(28);
    this->verticalHeader()->setMinimumSectionSize(28);

    this->setAlternatingRowColors(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    return;
}

void PropertyTable::modify_property(QString propertyName, QString value)
{
    // Row index in the table of the item with the given property name.
    int rowIdx = 0;

    // See if a property with the give name altready exists in the table.
    for (rowIdx = 0; rowIdx <  this->rowCount(); rowIdx++)
    {
        if (this->item(rowIdx, 0)->text() == propertyName)
        {
            goto modify_property;
        }
    }

    // If a property with the given name doesn't already exist, we'll add it.
    {
        rowIdx = this->rowCount();
        this->insertRow(this->rowCount());
        this->setItem(rowIdx, 0, new QTableWidgetItem(propertyName));
    }

    modify_property:
    this->setItem(rowIdx, 1, new QTableWidgetItem(value));

    return;
}
