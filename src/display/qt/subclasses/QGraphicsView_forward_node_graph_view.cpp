#include <QGraphicsProxyWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QDebug>
#include "display/qt/subclasses/QGraphicsView_forward_node_graph_view.h"
#include "display/qt/subclasses/QGraphicsItem_forward_node_graph_node.h"

ForwardNodeGraphView::ForwardNodeGraphView(QWidget *parent) : QGraphicsView(parent)
{
    return;
}

void ForwardNodeGraphView::mousePressEvent(QMouseEvent *event)
{
    // Dragging with the middle button will translate the view.
    if (event->button() == Qt::MiddleButton)
    {
        lastKnownMouseMiddleDragPos = event->pos();

        return;
    }

    QGraphicsView::mousePressEvent(event);

    return;
}

void ForwardNodeGraphView::mouseMoveEvent(QMouseEvent *event)
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

void ForwardNodeGraphView::wheelEvent(QWheelEvent *event)
{
    // Scrolling the mouse wheel while holding down control scales the view.
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
        {
            this->scale(1.2, 1.2);
        }
        else
        {
            this->scale(0.8, 0.8);
        }

        // Don't let the event propagate any further. This prevents e.g. the wheel from
        // inadvertently engaging an underlying GUI element.
        return;
    }

    QGraphicsView::wheelEvent(event);

    return;
}
