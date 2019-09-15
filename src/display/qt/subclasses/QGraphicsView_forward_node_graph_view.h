#ifndef FORWARD_NODE_GRAPH_VIEW_H
#define FORWARD_NODE_GRAPH_VIEW_H

#include <QGraphicsView>
#include <functional>

class ForwardNodeGraphNode;
class QMenu;

struct node_edge_s;

class ForwardNodeGraphView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ForwardNodeGraphView(QWidget *parent = 0);

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void populate_node_click_menu(ForwardNodeGraphNode *const node);
    void populate_edge_click_menu(node_edge_s *const edge);

    // Stores the last known position of the mouse cursor when it was moved while
    // a button was simultaneously held down.
    QPointF lastKnownMouseMiddleDragPos;

    QMenu *nodeClickMenu = nullptr;
    QMenu *edgeClickMenu = nullptr;
};

#endif
