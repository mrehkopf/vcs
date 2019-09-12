/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A base node for a forward node graph; which itself inherits from QGraphicsItem.
 *
 * You can subclass ForwardNodeGraphNode to create custom nodes for the graph. The important
 * functions to override are paint() and boundingRect(). Please see Qt's documentation on
 * QGraphicsItem for more info on these functions and others that may be relevant.
 *
 */

#include "QGraphicsItem_forward_node_graph_node.h"

// Returns a pointer to the first edge on this node that is intersected by the given
// point; or nullptr if such an edge cannot be found.
node_edge_s *ForwardNodeGraphNode::intersected_edge(const QPointF &point)
{
    const QPointF relativePoint = this->mapFromScene(point);

    for (auto &edge: this->edges)
    {
        if (edge.rect.contains(QPoint(relativePoint.x(), relativePoint.y())))
        {
            return &edge;
        }
    }

    return nullptr;
}

// Connect this edge to the given target edge. If this is an input node, the
// connection will be made through the target edge, instead (assuming it's an
// output edge).
//
// Returns true if the connection succeeds; false otherwise.
//
bool node_edge_s::connect_to(node_edge_s * const targetEdge)
{
    if (!(this->direction ^ targetEdge->direction))
    {
        qCritical() << "There must be one input edge and one output edge for forming a connection.";
        return false;
    }

    if (targetEdge->parentNode == this->parentNode)
    {
        qCritical() << "Cannot connect a node to itself.";
        return false;
    }

    if (targetEdge->direction == this->direction)
    {
        qCritical() << "Cannot connect an edge to another of the same direction.";
        return false;
    }

    if (this->direction != direction_e::out)
    {
        return targetEdge->connect_to(this);
    }

    // Connect. If this connection already exists, update it; otherwise, add it.
    const auto existingConnection = std::find(this->outgoingConnections.begin(), this->outgoingConnections.end(), targetEdge);
    if (existingConnection != this->outgoingConnections.end())
    {
        (*existingConnection) = targetEdge;
    }
    else
    {
        this->outgoingConnections.push_back(targetEdge);
    }

    return true;
}

// Disconnect this edge from the given target edge. If this is an input edge,
// the disconnection will be made via the target edge, instead (assuming it's
// an output edge).
//
// Returns true if the disconnection succeeds; false otherwise.
//
bool node_edge_s::disconnect_from(node_edge_s * const targetEdge)
{
    if (!(this->direction ^ targetEdge->direction))
    {
        qCritical() << "There must be one input edge and one output edge for disconnecting.";
        return false;
    }

    if (this->direction != direction_e::out)
    {
        return targetEdge->disconnect_from(this);
    }

    // Disconnect.
    const auto existingConnection = std::find(this->outgoingConnections.begin(), this->outgoingConnections.end(), targetEdge);
    if (existingConnection != this->outgoingConnections.end())
    {
        this->outgoingConnections.erase(existingConnection);
    }

    return true;
}
