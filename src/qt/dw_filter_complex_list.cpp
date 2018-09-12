/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter complex list
 *
 * Extends the QListWidget widget to add right-click-deleting of items.
 *
 */

#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QMenu>
#include "dw_filter_complex_list.h"
#include "d_filter_complex_dialog.h"
#include "../common.h"
#include "../filter.h"

static QMenu *DELETE_ITEM_MENU = nullptr;

static QListWidgetItem *CURRENT_ITEM = nullptr;

Widget_FilterComplexList::Widget_FilterComplexList(QWidget *parent) : QListWidget(parent)
{
    DELETE_ITEM_MENU = new QMenu(this);
    DELETE_ITEM_MENU->addAction("Environment");
    DELETE_ITEM_MENU->addSeparator();
    DELETE_ITEM_MENU->addAction("Delete this filter complex", this, SLOT(remove_current_complex()));
    DELETE_ITEM_MENU->actions().at(0)->setEnabled(false);

    return;
}

// Removes all filter complexes from the filterer and re-adds them based on which
// filter complexes we have in the GUI list. You'd call this after you've
// deleted an item off the list in the GUI.
//
void Widget_FilterComplexList::refresh_filter_complex_list()
{
    std::vector<filter_complex_s> complexes = kf_filter_complexes();

    kf_clear_filters();

    for (int i = 0; i < this->count(); i++)
    {
        QStringList sl = item(i)->text().split(' ');
        resolution_s inR, outR;

        inR.w = sl.at(0).toUInt();
        inR.h = sl.at(2).toUInt();
        outR.w = sl.at(4).toUInt();
        outR.h = sl.at(6).toUInt();

        // Find the existing filter with this resolution and re-add it to the
        // known filters list.
        for (auto &f: complexes)
        {
            if (f.inRes.w == inR.w &&
                f.inRes.h == inR.h &&
                f.outRes.w == outR.w &&
                f.outRes.h == outR.h)
            {
                kf_update_filter_complex(f);

                break;
            }
        }
    }

    return;
}

// Removes the currently-selected filter complex from the list.
//
void Widget_FilterComplexList::remove_current_complex()
{
   // DEBUG(("Deleting an item (%p) from the filter complex list.", CURRENT_ITEM));

    if (CURRENT_ITEM == nullptr)
    {
        return;
    }

    for (int i = 0; i < this->count(); i++)
    {
        if (this->item(i) == CURRENT_ITEM)
        {
            delete item(i);
            refresh_filter_complex_list();

            return;
        }
    }

    NBENE(("Failed to delete the selected filter complex."));

    return;
}

// For opening a menu on right-clicking.
//
void Widget_FilterComplexList::contextMenuEvent(QContextMenuEvent *e)
{
    CURRENT_ITEM = itemAt(e->pos());    // The item that was clicked on.

    if (CURRENT_ITEM == nullptr)
    {
        return;
    }

    // Ignore events other than those triggered by the mouse right-click.
    if (e->reason() != QContextMenuEvent::Mouse)
    {
        return;
    }

    DELETE_ITEM_MENU->actions().at(0)->setText(CURRENT_ITEM->text());
    DELETE_ITEM_MENU->popup(mapToGlobal(e->pos()) + QPoint(10, 10));

    return;
}
