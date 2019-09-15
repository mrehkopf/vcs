/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef NODE_GRAPH_NODE_H
#define NODE_GRAPH_NODE_H

#include <QGraphicsItem>
#include <QDebug>
#include <algorithm>

class ForwardNodeGraphNode;

struct node_edge_s
{
    // A bounding rectangle for this edge's geometry.
    QRect rect = QRect();

    // The node this edge belongs to.
    ForwardNodeGraphNode *parentNode = nullptr;

    // All connections from this edge out to others. Note that only output edges
    // will have outgoing connections; for input edges, this vector will remain empty.
    std::vector<node_edge_s*> outgoingConnections;

    // Whether this edge flows data in or out.
    enum direction_e { in = 0, out = !in } direction;

    node_edge_s(const direction_e direction, const QRect rect, ForwardNodeGraphNode *const parent);
    bool connect_to(node_edge_s *const targetEdge);
    bool disconnect_from(node_edge_s *const targetEdge);
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

class ForwardNodeGraphNode : public QGraphicsItem
{
public:
    ForwardNodeGraphNode(const QString title,
                         const unsigned width = 260,
                         const unsigned height = 130) :
        width(width),
        height(height),
        title(title)
    {
        return;
    }

    node_edge_s* intersected_edge(const QPointF &point);

    const unsigned width;
    const unsigned height;
    const QString title;

protected:
    std::vector<node_edge_s> edges;

private:
};

#endif
