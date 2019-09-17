#ifndef FILTERS_DIALOG_NODES_H
#define FILTERS_DIALOG_NODES_H

#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"

class filter_c;

class FilterGraphNode : public InteractibleNodeGraphNode
{
public:
    FilterGraphNode(const QString title,
                    const unsigned width = 240,
                    const unsigned height = 130);
    virtual ~FilterGraphNode();

    const filter_c *associatedFilter = nullptr;

    // Convenience functions that can be used to access the node's (default) input and output edge.
    virtual node_edge_s* input_edge(void) { return nullptr; }
    virtual node_edge_s* output_edge(void) { return nullptr; }

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

    QRectF boundingRect(void) const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    node_edge_s* input_edge(void) override;
    node_edge_s* output_edge(void) override;

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

    QRectF boundingRect(void) const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    node_edge_s* output_edge(void) override;

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

    QRectF boundingRect(void) const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    node_edge_s* input_edge(void) override;

private:
};

#endif
