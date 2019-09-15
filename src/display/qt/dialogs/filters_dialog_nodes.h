#ifndef FILTERS_DIALOG_NODES_H
#define FILTERS_DIALOG_NODES_H

#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"

class filter_c;

class FilterGraphNode : public InteractibleNodeGraphNode
{
public:
    FilterGraphNode(const QString title,
                    const unsigned width = 240,
                    const unsigned height = 130) : InteractibleNodeGraphNode(title, width, height)
    {
        return;
    }

    const filter_c *associatedFilter;

    // Convenience functions that can be used to access the node's (default) input and output edge.
    virtual const node_edge_s& input_edge() const { return this->edges.at(-1); }
    virtual const node_edge_s& output_edge() const { return this->edges.at(-1); }

private:
};

class FilterNode : public FilterGraphNode
{
public:
    FilterNode(const QString title,
               const unsigned width = 240,
               const unsigned height = 130) : FilterGraphNode(title, width, height)
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

    const node_edge_s& input_edge() const override;
    const node_edge_s& output_edge() const override;

private:
};

class InputGateNode : public FilterGraphNode
{
public:
    InputGateNode(const QString title,
                  const unsigned width = 200,
                  const unsigned height = 130) : FilterGraphNode(title, width, height)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-9, 10, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const node_edge_s& output_edge() const override;

private:
};

class OutputGateNode : public FilterGraphNode
{
public:
    OutputGateNode(const QString title,
                   const unsigned width = 200,
                   const unsigned height = 130) : FilterGraphNode(title, width, height)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::in, QRect(-9, 10, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const node_edge_s& input_edge() const override;

private:
};

#endif
