#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QAction>
#include <QDebug>
#include "display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
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
            const QColor backgroundColor = ([=]()->QColor
            {
                const int alpha = ((option->state & QStyle::State_Selected)? 210 : 170);

                if      (this->backgroundColor == "Cyan")    return QColor(90, 190, 190, alpha);
                else if (this->backgroundColor == "Red")     return QColor(190, 90, 90, alpha);
                else if (this->backgroundColor == "Green")   return QColor(90, 190, 90, alpha);
                else if (this->backgroundColor == "Blue")    return QColor(90, 90, 190, alpha);
                else if (this->backgroundColor == "Yellow")  return QColor(190, 190, 90, alpha);
                else if (this->backgroundColor == "Magenta") return QColor(190, 90, 190, alpha);
                else                                         return QColor(90, 190, 190, alpha);
            })();

            painter->setPen(QPen(QColor("black"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(backgroundColor));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("white"));
            painter->setBrush(QBrush(QColor("lightgray")));

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
            painter->setPen(QPen(QColor("black"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(90, 90, 90, (option->state & QStyle::State_Selected)? 175 : 125)));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("white"));
            painter->setBrush(QBrush(QColor("lightgray")));

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
            painter->setPen(QPen(QColor("black"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(90, 90, 90, (option->state & QStyle::State_Selected)? 175 : 125)));
            painter->drawRoundedRect(0, 0, this->width, this->height, 2, 2);
        }

        // Connection points (edges).
        {
            painter->setPen(QColor("white"));
            painter->setBrush(QBrush(QColor("lightgray")));

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

node_edge_s* OutputGateNode::input_edge(void)
{
    node_edge_s &inputEdge = this->edges[0];

    k_assert((inputEdge.direction == node_edge_s::direction_e::in), "Unexpected type of input edge.");

    return &inputEdge;
}

FilterGraphNode::FilterGraphNode(const QString title,
                                 const unsigned width,
                                 const unsigned height,
                                 const bool isGateNode) : InteractibleNodeGraphNode(title, width, height)
{
    this->isGateNode = isGateNode;
    this->generate_right_click_menu();

    return;
}

FilterGraphNode::~FilterGraphNode()
{
    kf_delete_filter_instance(this->associatedFilter);

    this->rightClickMenu->close();
    this->rightClickMenu->deleteLater();

    return;
}

void FilterGraphNode::set_background_color(const QString colorName)
{
    // If we recognize this color name.
    if (this->backgroundColorList.indexOf(colorName) >= 0)
    {
        this->backgroundColor = colorName;
        this->update();
    }

    return;
}

const QList<QString>& FilterGraphNode::background_color_list(void)
{
    return this->backgroundColorList;
}

const QString& FilterGraphNode::current_background_color_name(void)
{
    return this->backgroundColor;
}

void FilterGraphNode::generate_right_click_menu(void)
{
    if (this->rightClickMenu)
    {
        this->rightClickMenu->deleteLater();
    }

    this->rightClickMenu = new QMenu();

    this->rightClickMenu->addAction(this->title);
    this->rightClickMenu->actions().at(0)->setEnabled(false);

    // Add options to change the node's color.
    if (!this->background_color_list().empty())
    {
        this->rightClickMenu->addSeparator();

        QMenu *colorMenu = new QMenu("Background color", this->rightClickMenu);

        QActionGroup *colorGroup = new QActionGroup(colorMenu);
        colorGroup->setExclusive(true);

        for (const auto &color : this->backgroundColorList)
        {
            QAction *colorAction = new QAction(color, colorMenu);
            colorAction->setCheckable(true);
            colorAction->setChecked(color == this->backgroundColor);
            colorAction->setActionGroup(colorGroup);
            colorMenu->addAction(colorAction);

            connect(colorAction, &QAction::triggered, this, [=]
            {
                this->set_background_color(color);
            });
        }

        this->rightClickMenu->addMenu(colorMenu);
    }

    // Add the option to delete this node.
    {
        this->rightClickMenu->addSeparator();

        connect(this->rightClickMenu->addAction("Remove this node"), &QAction::triggered, this, [=]
        {
            dynamic_cast<InteractibleNodeGraph*>(this->scene())->remove_node(this);
        });
    }

    return;
}
