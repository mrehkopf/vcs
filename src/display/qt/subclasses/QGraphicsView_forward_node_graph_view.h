#ifndef FORWARD_NODE_GRAPH_VIEW_H
#define FORWARD_NODE_GRAPH_VIEW_H

#include <QGraphicsView>

class ForwardNodeGraphView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ForwardNodeGraphView(QWidget *parent = 0);

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    // Stores the last known position of the mouse cursor when it was moved while
    // a button was simultaneously held down.
    QPointF lastKnownMouseMiddleDragPos;
};

#endif
