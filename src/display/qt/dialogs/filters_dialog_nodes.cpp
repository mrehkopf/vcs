#include <QPainter>
#include <QDebug>
#include "display/qt/dialogs/filters_dialog_nodes.h"
#include "common/globals.h"

QRectF FilterNode::boundingRect() const
{
    const int penThickness = 1;

    return QRectF(-10, -penThickness, (this->width + 20), (this->height + penThickness));
}

void FilterNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
            // Body background.
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor(QColor(90, 90, 90))));
            painter->drawRoundedRect(0, 0, this->width, this->height, 0, 0);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                if (edge.direction == node_edge_s::direction_e::out)
                {
                    painter->drawRoundedRect(edge.rect, 2, 2);
                }
                else
                {
                    painter->drawEllipse(edge.rect);
                }
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

const node_edge_s &FilterNode::input_edge() const
{
    const node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return inputEdge;
}

const node_edge_s &FilterNode::output_edge() const
{
    const node_edge_s &outputEdge = this->edges[1];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return outputEdge;
}

QRectF InputGateNode::boundingRect() const
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
            // Body background.
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor(QColor(125, 75, 120))));
            painter->drawRoundedRect(0, 0, this->width, this->height, 0, 0);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                if (edge.direction == node_edge_s::direction_e::out)
                {
                    painter->drawRoundedRect(edge.rect, 2, 2);
                }
                else
                {
                    painter->drawEllipse(edge.rect);
                }
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

const node_edge_s &InputGateNode::output_edge() const
{
    const node_edge_s &outputEdge = this->edges[0];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return outputEdge;
}

QRectF OutputGateNode::boundingRect() const
{
    const int penThickness = 1;

    return QRectF(-10, -penThickness, (this->width + 20), (this->height + penThickness));
}

void OutputGateNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
            // Body background.
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor(QColor(100, 75, 125))));
            painter->drawRoundedRect(0, 0, this->width, this->height, 0, 0);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("black"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                if (edge.direction == node_edge_s::direction_e::out)
                {
                    painter->drawRoundedRect(edge.rect, 2, 2);
                }
                else
                {
                    painter->drawEllipse(edge.rect);
                }
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

const node_edge_s &OutputGateNode::input_edge() const
{
    const node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return inputEdge;
}
