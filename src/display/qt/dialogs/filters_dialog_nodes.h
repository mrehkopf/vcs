#ifndef FILTERS_DIALOG_NODES_H
#define FILTERS_DIALOG_NODES_H

#include "display/qt/subclasses/QGraphicsItem_forward_node_graph_node.h"

class FilterNode : public ForwardNodeGraphNode
{
public:
    FilterNode(const QString title) : ForwardNodeGraphNode(title, 260, 130)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::in, QRect(-9, 10, 18, 18), this),
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-9, 10, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const node_edge_s& input_edge() const;
    const node_edge_s& output_edge() const;

private:
};

class InputGateNode : public ForwardNodeGraphNode
{
public:
    InputGateNode() : ForwardNodeGraphNode("Input gate", 200, 100)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-9, 10, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const node_edge_s& output_edge() const;

private:
};

class OutputGateNode : public ForwardNodeGraphNode
{
public:
    OutputGateNode() : ForwardNodeGraphNode("Output gate", 200, 100)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::in, QRect(-9, 10, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const node_edge_s& input_edge() const;

private:
};

#endif
