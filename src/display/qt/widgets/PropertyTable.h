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

class Overlay;

class PropertyTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit PropertyTable(QWidget *parent = 0);

    // Adds a property with the given name and value; or if a property by this
    // name already exists in the table, modifies its value.
    void modify_property(const QString &propertyName, const QString &value);

    unsigned add_property(const QString &name, const QString &tooltip = "");

    int find_row_idx(const QString &name);

    void reset_property_values(void);

private:
};

#endif
