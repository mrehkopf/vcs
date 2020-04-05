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

    void set_scale(const double newScale);

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
    const double defaultViewScale = 1;
    const double viewScaleStepSize = 0.15;
    const double minViewScale = this->viewScaleStepSize;
    const double maxViewScale = 1.95;
    double viewScale = this->defaultViewScale;
};

#endif
