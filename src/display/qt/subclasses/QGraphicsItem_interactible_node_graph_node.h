/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QGRAPHICSITEM_INTERACTIBLE_NODE_GRAPH_NODE_H
#define VCS_DISPLAY_QT_SUBCLASSES_QGRAPHICSITEM_INTERACTIBLE_NODE_GRAPH_NODE_H

#include <QGraphicsItem>
#include <QDebug>
#include <QMenu>
#include <algorithm>

class InteractibleNodeGraphNode;

struct node_edge_s
{
    // A bounding rectangle for this edge's geometry.
    QRect rect = QRect();

    // The node this edge belongs to.
    InteractibleNodeGraphNode *parentNode = nullptr;

    // All connections between this edge and others.
    std::vector<node_edge_s*> connectedTo;

    // Whether this edge flows data in or out.
    enum direction_e { in = 0, out = !in } direction;

    node_edge_s(const direction_e direction, const QRect rect, InteractibleNodeGraphNode *const parent);
    bool connect_to(node_edge_s *const targetEdge, const bool recursed = false);
    bool disconnect_from(node_edge_s *const targetEdge, const bool recursed = false);
};

// A connection between two node edges.
struct node_edge_connection_s
{
    const node_edge_s *edge1 = nullptr;
    const node_edge_s *edge2 = nullptr;

    QGraphicsLineItem *line = nullptr;

    node_edge_connection_s(const node_edge_s *const edge1,
                           const node_edge_s *const edge2,
                           QGraphicsLineItem *const line)
    {
        this->edge1 = edge1;
        this->edge2 = edge2;
        this->line = line;

        return;
    }
};

struct node_connection_event_s
{
    // The edge the connection originates from.
    node_edge_s *sourceEdge = nullptr;

    // The current mouse position while this connection event is ongoing. Used to
    // draw a line from the source node to the cursor.
    QPointF mousePos = QPointF();

    // A line drawn in the graphics scene between the source edge and the current
    // position of the user's mouse cursor when no target edge has yet been defined.
    QGraphicsLineItem *graphicsLine = nullptr;
};

class InteractibleNodeGraphNode : public QGraphicsItem
{
public:
    InteractibleNodeGraphNode(const QString title,
                              const unsigned width = 260,
                              const unsigned height = 130) :
        width(width),
        height(height),
        title(title)
    {
        return;
    }

    node_edge_s* intersected_edge(const QPointF &point);
    void disconnect_all_edges(void);

    const unsigned width;
    const unsigned height;
    const QString title;

    // A menu that opens when this node is right-clicked (e.g. within its parent
    // graphics scene).
    QMenu *rightClickMenu = nullptr;

protected:
    std::vector<node_edge_s> edges;

private:
};

#endif
