/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include <QDropEvent>
#include "dw_filter_list.h"

Widget_FilterList::Widget_FilterList(QWidget *parent) : QListWidget(parent)
{
    return;
}

void Widget_FilterList::dropEvent(QDropEvent *event)
{
    // Just say we accept the event and then ignore it.
    /// In hindsight, I forget what this was useful for, if anything.
    event->accept();

    return;
}
