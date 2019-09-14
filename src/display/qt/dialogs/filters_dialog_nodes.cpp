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
    titleFont.setPointSize(bodyFont.pointSize()+1);

    // Draw the node's body.
    {
        painter->setFont(bodyFont);

        // Background.
        {
            // Body background.
            painter->setBrush(QBrush(QColor(127, 127, 127)));
            painter->setPen(QColor("transparent"));
            painter->drawRoundedRect(0, 0, this->width, this->height, 4, 4);

            // Title background.
            painter->setBrush(QBrush(QColor(QColor(90, 90, 90))));
            painter->drawRoundedRect(0, 0, this->width, 40, 4, 4);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                painter->drawEllipse(edge.rect);
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
    titleFont.setPointSize(bodyFont.pointSize()+1);

    // Draw the node's body.
    {
        painter->setFont(bodyFont);

        // Background.
        {
            // Body background.
            painter->setBrush(QBrush(QColor(127, 127, 127)));
            painter->setPen(QColor("transparent"));
            painter->drawRoundedRect(0, 0, this->width, this->height, 4, 4);

            // Title background.
            painter->setBrush(QBrush(QColor(QColor(75, 75, 150))));
            painter->drawRoundedRect(0, 0, this->width, 40, 4, 4);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                painter->drawEllipse(edge.rect);
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
    titleFont.setPointSize(bodyFont.pointSize()+1);

    // Draw the node's body.
    {
        painter->setFont(bodyFont);

        // Background.
        {
            // Body background.
            painter->setBrush(QBrush(QColor(127, 127, 127)));
            painter->setPen(QColor("transparent"));
            painter->drawRoundedRect(0, 0, this->width, this->height, 4, 4);

            // Title background.
            painter->setBrush(QBrush(QColor(QColor(150, 75, 75))));
            painter->drawRoundedRect(0, 0, this->width, 40, 4, 4);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("mediumseagreen")));

            for (const auto &edge: this->edges)
            {
                painter->drawEllipse(edge.rect);
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
