/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTERCOMPLEXLIST_H
#define FILTERCOMPLEXLIST_H

#include <QListWidget>

class Widget_FilterComplexList : public QListWidget
{
    Q_OBJECT

public:
    explicit Widget_FilterComplexList(QWidget *parent = 0);

signals:

public slots:

private slots:
    void remove_current_complex();

private:
    void contextMenuEvent(QContextMenuEvent *e);

    void refresh_filter_complex_list();
};

#endif // FILTERCOMPLEXLIST_H
