#ifndef FILTER_GRAPH_NODES_H_
#define FILTER_GRAPH_NODES_H_

#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"

class filter_c;

class FilterGraphNode : public QObject, public InteractibleNodeGraphNode
{
    Q_OBJECT

public:
    FilterGraphNode(const QString title,
                    const unsigned width = 240,
                    const unsigned height = 130,
                    const bool isGateNode = false);
    virtual ~FilterGraphNode();

    const filter_c *associatedFilter = nullptr;

    // Convenience functions that can be used to access the node's (default) input and output edge.
    virtual node_edge_s* input_edge(void) { return nullptr; }
    virtual node_edge_s* output_edge(void) { return nullptr; }

    void set_background_color(const QString colorName);
    const QList<QString>& background_color_list(void);
    const QString& current_background_color_name(void);

protected:
    // Whether this node is an input or output gate.
    bool isGateNode = false;

    // A list of the node background colors we support. Note: You shouldn't
    // delete or change the names of any of the items on this list. But
    // you can add more.
    const QList<QString> backgroundColorList = {
        "Blue", "Cyan", "Green", "Magenta", "Red", "Yellow",
    };

    QString backgroundColor = this->backgroundColorList.at(1);

private:
    void generate_right_click_menu(void);
};

class FilterNode : public FilterGraphNode
{
public:
    FilterNode(const QString title,
               const unsigned width = 240,
               const unsigned height = 130) : FilterGraphNode(title, width, height, false)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::in, QRect(-10, 11, 18, 18), this),
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-10, 11, 18, 18), this),
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
                  const unsigned height = 130) : FilterGraphNode(title, width, height, true)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::out, QRect(this->width-10, 11, 18, 18), this),
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
                   const unsigned height = 130) : FilterGraphNode(title, width, height, true)
    {
        this->edges =
        {
            node_edge_s(node_edge_s::direction_e::in, QRect(-10, 11, 18, 18), this),
        };

        return;
    }

    QRectF boundingRect(void) const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    node_edge_s* input_edge(void) override;

private:
};

#endif
