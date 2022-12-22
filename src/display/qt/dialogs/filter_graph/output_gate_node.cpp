/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QStyle>
#include "common/assert.h"
#include "display/qt/dialogs/filter_graph/output_gate_node.h"

node_edge_s* OutputGateNode::input_edge(void)
{
    node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return &inputEdge;
}
