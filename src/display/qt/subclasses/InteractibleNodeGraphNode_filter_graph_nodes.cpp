#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QDebug>
#include "display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.h"
#include "common/globals.h"
#include "filter/filter.h"

QRectF FilterNode::boundingRect(void) const
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
            painter->setPen(QPen(QColor((option->state & QStyle::State_Selected)? "orange" : "transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(QColor(90, 90, 90))));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("orange")));

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

node_edge_s* FilterNode::input_edge(void)
{
    node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return &inputEdge;
}

node_edge_s* FilterNode::output_edge(void)
{
    node_edge_s &outputEdge = this->edges[1];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return &outputEdge;
}

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
            // Body background.
            painter->setPen(QPen(QColor((option->state & QStyle::State_Selected)? "orange" : "transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(QColor(90, 90, 90)), Qt::Dense1Pattern));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("orange")));

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

node_edge_s* InputGateNode::output_edge(void)
{
    node_edge_s &outputEdge = this->edges[0];

    k_assert((outputEdge.direction == node_edge_s::direction_e::out), "Unexpected type of output edge.");

    return &outputEdge;
}

QRectF OutputGateNode::boundingRect(void) const
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
            painter->setPen(QPen(QColor((option->state & QStyle::State_Selected)? "orange" : "transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(QColor(90, 90, 90)), Qt::Dense1Pattern));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("transparent"));
            painter->setBrush(QBrush(QColor("orange")));

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

node_edge_s* OutputGateNode::input_edge(void)
{
    node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return &inputEdge;
}

FilterGraphNode::FilterGraphNode(const QString title,
                                 const unsigned width,
                                 const unsigned height) : InteractibleNodeGraphNode(title, width, height)
{
    return;
}

FilterGraphNode::~FilterGraphNode()
{
    kf_delete_filter_instance(this->associatedFilter);

    return;
}
