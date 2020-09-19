/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QStyle>
#include "filter/filter.h"
#include "common/globals.h"
#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"

FilterGraphNode::FilterGraphNode(const filter_node_type_e type,
                                 const QString title,
                                 const unsigned width,
                                 const unsigned height) :
    InteractibleNodeGraphNode(title, width, height),
    filterType(type)
{
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

QRectF FilterGraphNode::boundingRect(void) const
{
    const int margin = 20;

    return QRectF(-margin,
                  -margin,
                  (this->width + (margin * 2)),
                  (this->height + (margin * 2)));
}

void FilterGraphNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)option;
    (void)widget;

    QFont bodyFont = painter->font();
    QFont titleFont = painter->font();

    painter->setRenderHint(QPainter::Antialiasing, false);

    const auto draw_box_shadow = [painter](const QRect &rect, const unsigned lineWidth)
    {
        painter->setPen(QPen(QColor("#f5f5f0"), lineWidth, Qt::SolidLine));
        painter->drawLine(rect.x(),
                          rect.y(),
                          rect.x(),
                          rect.y() + (rect.height() - 1));
        painter->drawLine(rect.x(),
                          rect.y(),
                          rect.x() + (rect.width() - 1),
                          rect.y());

        painter->setPen(QPen(QColor("#808075"), lineWidth, Qt::SolidLine));
        painter->drawLine(rect.x() + (rect.width()),
                          rect.y(),
                          rect.x() + (rect.width()),
                          rect.y() + (rect.height()));
        painter->drawLine(rect.x(),
                          rect.y() + (rect.height()),
                          rect.x() + (rect.width()),
                          rect.y() + (rect.height()));
    };

    // Draw the node's body.
    {
        const unsigned edgePenThickness = 4;

        painter->setFont(bodyFont);

        // Background.
        {
            // Node's background.
            painter->setPen(QPen(QColor("transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor("#d4d0c8")));
            painter->drawRect(0, 0, this->width, this->height);
            draw_box_shadow(QRect(0, 0, this->width, this->height), edgePenThickness);

            // Title bar's background.
            painter->setPen(QPen(QColor("transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush((this->is_enabled()? this->current_background_color() : "#a5a39f"),
                                     (this->is_enabled()? Qt::SolidPattern : Qt::BDiagPattern)));
            painter->drawRect(QRect(2, 8, (this->width - 4), 24));
        }

        // Connection points (edges).
        {
            for (const auto &edge: this->edges)
            {
                painter->setPen(QColor("#d4d0c8"));
                painter->setBrush(QBrush(QColor("#d4d0c8")));
                painter->drawRect(edge.rect);
                draw_box_shadow(edge.rect, (edgePenThickness / 2));
            }
        }
    }

    const QString clr = (this->current_background_color().lightness() < 128? "white" : "black");
    painter->setFont(titleFont);
    painter->setPen(QColor(this->is_enabled()? clr : "black"));
    painter->drawText(20, 26, title);

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

const QColor FilterGraphNode::current_background_color(void)
{
    const QString currentColor = this->backgroundColor.toLower();

    if (currentColor == "red") return QColor("brown");
    else if (currentColor == "black") return QColor("dimgray");
    else if (currentColor == "blue") return QColor("navy");
    else if (currentColor == "cyan") return QColor("darkcyan");
    else if (currentColor == "magenta") return QColor("mediumvioletred");

    return this->backgroundColor;
}

bool FilterGraphNode::is_enabled(void) const
{
    return this->isEnabled;
}

void FilterGraphNode::set_enabled(const bool enabled)
{
    this->isEnabled = enabled;

    if (enabled)
    {

        emit this->enabled();
    }
    else
    {
        emit this->disabled();
    }

    dynamic_cast<InteractibleNodeGraph*>(this->scene())->update();

    return;
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

    // Add an option to enable/disable this node. Nodes that are disabled will act
    // as passthroughs, and their corresponding filter won't be applied.
    if (this->filterType != filter_node_type_e::gate)
    {
        this->rightClickMenu->addSeparator();

        QAction *enabled = new QAction("Active", this->rightClickMenu);

        enabled->setCheckable(true);
        enabled->setChecked(this->isEnabled);

        connect(this, &FilterGraphNode::enabled, this, [=]
        {
            enabled->setChecked(true);
        });

        connect(this, &FilterGraphNode::disabled, this, [=]
        {
            enabled->setChecked(false);
        });

        connect(enabled, &QAction::triggered, this, [=]
        {
            this->set_enabled(!this->isEnabled);
            kd_recalculate_filter_graph_chains();
        });

        this->rightClickMenu->addAction(enabled);
    }

    // Add options to change the node's color.
    if (!this->background_color_list().empty())
    {
        this->rightClickMenu->addSeparator();

        QMenu *colorMenu = new QMenu("Background color", this->rightClickMenu);

        for (const auto &color : this->backgroundColorList)
        {
            QAction *colorAction = new QAction(color, colorMenu);
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
