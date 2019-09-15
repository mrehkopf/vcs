/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef NODE_GRAPH_H
#define NODE_GRAPH_H

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QDebug>
#include "QGraphicsItem_forward_node_graph_node.h"

class ForwardNodeGraph : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit ForwardNodeGraph(QObject *parent = 0);

private:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void complete_connection_event(node_edge_s *const finalEdge);
    void start_connection_event(node_edge_s *const sourceEdge, const QPointF mousePos);
    void reset_current_connection_event(void);
    void update_scene_connections(void);
    void connect_scene_edges(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge);

    node_connection_event_s connectionEvent;
    std::vector<node_edge_connection_s> edgeConnections;

signals:
    // Emitted when the user connects two edges in the scene.
    void newEdgeConnection(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge);
};

#endif
