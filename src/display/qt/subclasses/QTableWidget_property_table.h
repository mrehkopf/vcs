/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 * Provides a table with two columns: "Property" and "Value".
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QTABLEWIDGET_PROPERTY_TABLE_H
#define VCS_DISPLAY_QT_SUBCLASSES_QTABLEWIDGET_PROPERTY_TABLE_H

#include <QTableWidget>

class OverlayDialog;

class PropertyTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit PropertyTable(QWidget *parent = 0);

    // Adds a property with the given name and value; or if a property by this
    // name already exists in the table, modifies its value.
    void modify_property(QString propertyName, QString value);

private:
};

#endif
