#include <QGraphicsDropShadowEffect>
#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include "display/qt/subclasses/QGraphicsItem_forward_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_forward_node_graph.h"
#include "display/qt/dialogs/filters_dialog_nodes.h"
#include "display/qt/dialogs/filters_dialog.h"
#include "display/qt/widgets/filter_widgets.h"
#include "ui_filters_dialog.h"

FiltersDialog::FiltersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FiltersDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Filters");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(Qt::Window);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // File...
        {
            QMenu *fileMenu = new QMenu("File", this);

            menubar->addMenu(fileMenu);
        }

        // Add...
        {
            QMenu *addMenu = new QMenu("Add", this);

            // Insert the names of all available filter types.
            {
                std::vector<const filter_meta_s*> knownFilterTypes = kf_known_filter_types();

                std::sort(knownFilterTypes.begin(), knownFilterTypes.end(), [](const filter_meta_s *a, const filter_meta_s *b)
                {
                    return a->name < b->name;
                });

                // Add gates.
                for (const auto filter: knownFilterTypes)
                {
                    if ((filter->type != filter_type_enum_e::output_gate) &&
                        (filter->type != filter_type_enum_e::input_gate))
                    {
                        continue;
                    }

                    connect(addMenu->addAction(QString::fromStdString(filter->name)), &QAction::triggered,
                            [=]{add_filter_node(filter->type);});
                }

                addMenu->addSeparator();

                // Add filters.
                for (const auto filter: knownFilterTypes)
                {
                    if ((filter->type == filter_type_enum_e::output_gate) ||
                        (filter->type == filter_type_enum_e::input_gate))
                    {
                        continue;
                    }

                    connect(addMenu->addAction(QString::fromStdString(filter->name)), &QAction::triggered,
                            [=]{add_filter_node(filter->type);});
                }
            }

            menubar->addMenu(addMenu);
        }

        // Help...
        {
            QMenu *helpMenu = new QMenu("Help", this);

            helpMenu->addAction("About...");
            helpMenu->actions().at(0)->setEnabled(false); /// TODO: Add the actual help.

            menubar->addMenu(helpMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Create and configure the graphics scene.
    {
        this->graphicsScene = new ForwardNodeGraph(this);
        this->graphicsScene->setBackgroundBrush(QBrush("#353535"));

        ui->graphicsView->setScene(this->graphicsScene);

        connect(this->graphicsScene, &ForwardNodeGraph::edgesConnected, this, [this]{this->recalculate_filter_chains();});
    }

    // For temporary testing purposes, add some placeholder nodes into the graphics scene.
    {
        auto inputGateNode = this->add_filter_node(filter_type_enum_e::input_gate);
        auto outputGateNode = this->add_filter_node(filter_type_enum_e::output_gate);

        inputGateNode->moveBy(-200, 0);
        outputGateNode->moveBy(200, 0);
    }

    return;
}

FiltersDialog::~FiltersDialog()
{
    delete ui;

    return;
}

// Adds a new instance of the given filter type into the node graph. Returns a
// pointer to the new node.
FilterGraphNode* FiltersDialog::add_filter_node(const filter_type_enum_e type)
{
    const filter_c *const newFilter = kf_create_new_filter_instance(type);

    k_assert(newFilter, "Failed to add a new filter node.");

    FilterGraphNode *newNode = nullptr;

    switch (type)
    {
        case filter_type_enum_e::input_gate: newNode = new InputGateNode(newFilter->guiWidget->title); break;
        case filter_type_enum_e::output_gate: newNode = new OutputGateNode(newFilter->guiWidget->title); break;
        default: newNode = new FilterNode(newFilter->guiWidget->title); break;
    }

    newNode->associatedFilter = newFilter;
    newNode->setFlags(newNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
    this->graphicsScene->addItem(newNode);

    QGraphicsProxyWidget* nodeWidgetProxy = new QGraphicsProxyWidget(newNode);
    nodeWidgetProxy->setWidget(newFilter->guiWidget->widget);
    nodeWidgetProxy->widget()->move(10, 45);

    if (type == filter_type_enum_e::input_gate)
    {
        this->inputGateNodes.push_back(newNode);
    }

    ui->graphicsView->centerOn(newNode);

    return newNode;
}

// Visit each node in the graph and while doing so, group together such chains of
// filters that run from an input gate through one or more filters into an output
// gate. The chains will then be submitted to the filter handler.
void FiltersDialog::recalculate_filter_chains(void)
{
    kf_remove_all_filter_chains();

    const std::function<void(FilterGraphNode *const, std::vector<const filter_c*>)> traverse_filter_node =
          [&](FilterGraphNode *const node, std::vector<const filter_c*> accumulatedNodes)
    {
        k_assert(node, "Trying to visit an invalid node.");

        accumulatedNodes.push_back(node->associatedFilter);

        if (node->associatedFilter->metaData.type == filter_type_enum_e::output_gate)
        {
            kf_add_filter_chain(accumulatedNodes);
            return;
        }

        // NOTE: This assumes that each node in the graph only has one output edge.
        for (auto outgoing: node->output_edge().outgoingConnections)
        {
            traverse_filter_node(dynamic_cast<FilterGraphNode*>(outgoing->parentNode), accumulatedNodes);
        }

        return;
    };

    for (auto inputGate: this->inputGateNodes)
    {
        traverse_filter_node(inputGate, {});
    }

    return;
}
