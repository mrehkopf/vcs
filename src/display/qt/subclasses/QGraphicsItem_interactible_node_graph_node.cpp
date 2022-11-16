/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A base node for an interactible node graph, inheriting from QGraphicsItem.
 *
 * You can subclass InteractibleNodeGraphNode to create custom nodes for the graph.
 * The important functions to override are paint() and boundingRect(). Please see
 * Qt's documentation on QGraphicsItem for more info on these functions and others
 * that may be relevant.
 *
 */

#include <QApplication>
#include <cmath>
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"

// Returns a pointer to the first edge on this node that is intersected by the given
// point; or nullptr if such an edge cannot be found.
node_edge_s *InteractibleNodeGraphNode::intersected_edge(const QPointF &point)
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

void InteractibleNodeGraphNode::disconnect_all_edges(void)
{
    for (auto &edge: this->edges)
    {
        for (auto &connection: edge.connectedTo)
        {
            edge.disconnect_from(connection);
        }
    }

    return;
}

void InteractibleNodeGraphNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // If we were dragging any nodes, snap them to a grid.
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        InteractibleNodeGraph *const scene = dynamic_cast<InteractibleNodeGraph*>(this->scene());
        const double gridSize = double(scene->grid_size());

        for (auto *selectedItem: scene->selectedItems())
        {
            selectedItem->setPos(
                (gridSize * std::round(selectedItem->x() / gridSize)),
                (gridSize * std::round(selectedItem->y() / gridSize))
            );
        }

        scene->update_scene_connections();
    }

    QGraphicsItem::mouseReleaseEvent(event);

    return;
}

// Connect this edge to the given target edge. If this is an input node, the
// connection will be made through the target edge, instead (assuming it's an
// output edge).
//
// Returns true if the connection succeeds; false otherwise.
//
node_edge_s::node_edge_s(const node_edge_s::direction_e direction, const QRect rect, InteractibleNodeGraphNode *const parent)
{
    this->direction = direction;
    this->rect = rect;
    this->parentNode = parent;

    return;
}

// Connect this edge to the given target edge. Returns true if the connection is
// successfully established; false otherwise. Note that this function recursively
// modifies the target edge, as well, to mark it as being connected to the source
// edge.
bool node_edge_s::connect_to(node_edge_s *const targetEdge, const bool recursed)
{
    if (!(this->direction ^ targetEdge->direction))
    {
        qCritical() << "There must be one input edge and one output edge to form a connection.";
        return false;
    }

    if (targetEdge->parentNode == this->parentNode)
    {
        qCritical() << "Cannot connect a node to itself.";
        return false;
    }

    // Connect, if we're not already connected to the target.
    const auto existingConnection = std::find(this->connectedTo.begin(), this->connectedTo.end(), targetEdge);
    if (existingConnection == this->connectedTo.end())
    {
        this->connectedTo.push_back(targetEdge);

        auto *scene = dynamic_cast<InteractibleNodeGraph*>(this->parentNode->scene());
        if (scene && recursed)
        {
            scene->connect_scene_edges(this, targetEdge);
        }
    }

    return recursed? true : targetEdge->connect_to(this, true);
}

// Disconnect this edge from the given target edge. Returns true if the disconnection
// succeeds; false otherwise. Note that this function recursively modifies the target
// edge, as well, to mark it as being disconnected from the source edge.
bool node_edge_s::disconnect_from(node_edge_s *const targetEdge, const bool recursed)
{
    // Disconnect.
    const auto existingConnection = std::find(this->connectedTo.begin(), this->connectedTo.end(), targetEdge);
    if (existingConnection != this->connectedTo.end())
    {
        this->connectedTo.erase(existingConnection);

        auto *scene = dynamic_cast<InteractibleNodeGraph*>(this->parentNode->scene());
        if (scene && recursed)
        {
            scene->disconnect_scene_edges(this, targetEdge);
        }
    }

    return recursed? true : targetEdge->disconnect_from(this, true);
}
