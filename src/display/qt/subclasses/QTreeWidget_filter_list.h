/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTER_LIST_TREE_WIDGET_H
#define FILTER_LIST_TREE_WIDGET_H

#include <QTreeWidget>
#include "common/types.h"

struct legacy14_filter_s;

class FilterListTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    FilterListTreeWidget(QWidget *parent = 0);
    ~FilterListTreeWidget();

    std::vector<legacy14_filter_s> filters();

    void set_filters(const std::vector<legacy14_filter_s> &filters);

private:
    // Maps pointers to filter list items to data arrays holding the parameters
    // of each filter.
    std::map<const void*, u8*> filterData;
};

#endif
