/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTERLIST_H
#define FILTERLIST_H

#include <QListWidget>

class Widget_FilterList : public QListWidget
{
    Q_OBJECT

public:
    Widget_FilterList(QWidget *parent = 0);

private:
    void dropEvent(QDropEvent *event);
};

#endif // FILTERLIST_H
