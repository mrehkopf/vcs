/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <algorithm>
#include <QGraphicsProxyWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QDebug>
#include <QMenu>
#include "display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h"
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
#include "display/qt/keyboard_shortcuts.h"
#include "common/globals.h"
#include "filter/filter.h"

InteractibleNodeGraphView::InteractibleNodeGraphView(QWidget *parent) : QGraphicsView(parent)
{
    this->edgeClickMenu = new QMenu(this);
    this->setRenderHints(QPainter::Antialiasing);
    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // Set up keyboard shortcuts.
    {
        connect(kd_make_key_shortcut("filter-graph-dialog: delete-selected-nodes", this), &QShortcut::activated, this, [this]
        {
            for (QGraphicsItem *selectedItem: this->scene()->selectedItems())
            {
                dynamic_cast<InteractibleNodeGraph*>(this->scene())->remove_node(dynamic_cast<InteractibleNodeGraphNode*>(selectedItem));
            }
        });
    }

    return;
}

void InteractibleNodeGraphView::mousePressEvent(QMouseEvent *event)
{
    // Dragging with the middle button translates the view.
    if (event->button() == Qt::MiddleButton)
    {
        lastKnownMouseMiddleDragPos = event->pos();

        // Don't allow the event to propagate further.
        return;
    }
    // Pressing the right mouse button over a node or edge opens a context-sensitive popup menu.
    else if (event->button() == Qt::RightButton)
    {
        const QPointF scenePos = this->mapToScene(event->pos());

        // If this press was over a node edge, start an edge connection event.
        const auto itemsUnderCursor = this->scene()->items(scenePos, Qt::IntersectsItemBoundingRect);
        for (const auto item: itemsUnderCursor)
        {
            const auto nodeUnderCursor = dynamic_cast<InteractibleNodeGraphNode*>(item);

            if (nodeUnderCursor)
            {
                // Make sure only the clicked node is selected.
                this->scene()->setSelectionArea(QPainterPath());
                nodeUnderCursor->setSelected(true);

                node_edge_s *const edgeUnderCursor = nodeUnderCursor->intersected_edge(scenePos);

                if (edgeUnderCursor)
                {
                    this->populate_edge_click_menu(edgeUnderCursor);
                    this->edgeClickMenu->popup(this->mapToGlobal(event->pos()));
                }
                else
                {
                    nodeUnderCursor->rightClickMenu->popup(this->mapToGlobal(event->pos()));
                }

                // Don't allow the event to propagate further.
                return;
            }
        }

        // Otherwise, the click was over the background.
        emit this->right_clicked_on_background(event->globalPos());
    }
    // Pressing the left mouse button over the scene's background starts a rubberband selection.
    else if (event->button() == Qt::LeftButton)
    {
        // Only allow rubberband-dragging when the user clicks on the scene's background.
        if (this->scene()->itemAt(this->mapToScene(event->pos()), QTransform()))
        {
            this->setDragMode(QGraphicsView::NoDrag);
        }
        else
        {
            this->setDragMode(QGraphicsView::RubberBandDrag);
        }
    }

    QGraphicsView::mousePressEvent(event);

    return;
}

void InteractibleNodeGraphView::mouseMoveEvent(QMouseEvent *event)
{
    // Dragging with the middle button translates the view.
    if (event->buttons() & Qt::MiddleButton)
    {
        const QPointF movementDelta = (lastKnownMouseMiddleDragPos - event->pos());

        this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value() + movementDelta.x());
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + movementDelta.y());

        lastKnownMouseMiddleDragPos = event->pos();

        return;
    }

    QGraphicsView::mouseMoveEvent(event);

    return;
}

void InteractibleNodeGraphView::wheelEvent(QWheelEvent *event)
{
    // Scrolling the mouse wheel while holding down control scales the view.
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
        {
            this->viewScale += this->viewScaleStepSize;
        }
        else
        {
            this->viewScale -= this->viewScaleStepSize;
        }

        this->set_scale(this->viewScale);

        // Don't let the event propagate any further. This prevents e.g. the wheel from
        // inadvertently engaging an underlying GUI element.
        return;
    }

    QGraphicsView::wheelEvent(event);

    return;
}

void InteractibleNodeGraphView::populate_edge_click_menu(node_edge_s *const edge)
{
    k_assert(this->edgeClickMenu, "Expected a non-null edge click menu object.");

    this->edgeClickMenu->clear();

    this->edgeClickMenu->addAction("Disconnect from...");
    this->edgeClickMenu->actions().at(0)->setEnabled(false);

    this->edgeClickMenu->addSeparator();

    for (auto outgoing: edge->connectedTo)
    {
        connect(this->edgeClickMenu->addAction(outgoing->parentNode->title), &QAction::triggered, this, [=]
        {
            edge->disconnect_from(outgoing);
        });
    }

    return;
}

void InteractibleNodeGraphView::set_scale(double newScale)
{
    this->viewScale = std::max(this->minViewScale, std::min(this->maxViewScale, newScale));
    this->setTransform(QTransform(this->viewScale, 0, 0, this->viewScale, 0, 0));

    emit this->scale_changed(this->viewScale);

    return;
}
