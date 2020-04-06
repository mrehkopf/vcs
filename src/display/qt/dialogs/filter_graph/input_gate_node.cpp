/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QStyle>
#include "common/globals.h"
#include "display/qt/dialogs/filter_graph/input_gate_node.h"

QRectF InputGateNode::boundingRect(void) const
{
    const int penThickness = 1;

    return QRectF(-10, -penThickness, (this->width + 20), (this->height + penThickness));
}

void InputGateNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)option;
    (void)widget;

    QFont bodyFont = painter->font();
    QFont titleFont = painter->font();

    // Draw the node's body.
    {
        painter->setFont(bodyFont);

        // Background.
        {
            painter->setPen(QPen(QColor("black"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(90, 90, 90, (option->state & QStyle::State_Selected)? 175 : 125)));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("white"));
            painter->setBrush(QBrush(QColor("white")));

            for (const auto &edge: this->edges)
            {
                painter->drawRoundedRect(edge.rect, 2, 2);
            }
        }
    }

    // Draw the node's title text.
    {
        painter->setFont(titleFont);

        painter->setPen(QColor("white"));
        painter->drawText(20, 25, title);
    }

    return;
}

node_edge_s* InputGateNode::output_edge(void)
{
    node_edge_s &outputEdge = this->edges[0];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return &outputEdge;
}
