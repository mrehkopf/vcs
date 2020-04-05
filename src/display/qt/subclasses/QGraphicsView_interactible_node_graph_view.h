#ifndef INTERACTIBLE_NODE_GRAPH_VIEW_H
#define INTERACTIBLE_NODE_GRAPH_VIEW_H

#include <QGraphicsView>
#include <functional>

class InteractibleNodeGraphNode;
class QMenu;

struct node_edge_s;

class InteractibleNodeGraphView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit InteractibleNodeGraphView(QWidget *parent = 0);

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void populate_edge_click_menu(node_edge_s *const edge);

    // Stores the last known position of the mouse cursor when it was moved while
    // a button was simultaneously held down.
    QPointF lastKnownMouseMiddleDragPos;

    QMenu *edgeClickMenu = nullptr;

    // For zooming the view.
    double viewScale = 1;
    const double viewScaleStepSize = 0.2;
    const double minViewScale = this->viewScaleStepSize;
    const double maxViewScale = 2;
};

#endif
