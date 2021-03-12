#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "display/qt/dialogs/filter_graph/filter_node.h"
#include "display/qt/dialogs/filter_graph/input_gate_node.h"
#include "display/qt/dialogs/filter_graph/output_gate_node.h"
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/widgets/filter_widgets.h"
#include "display/qt/persistent_settings.h"
#include "common/disk/disk.h"
#include "ui_filter_graph_dialog.h"

FilterGraphDialog::FilterGraphDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterGraphDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(this->dialogBaseTitle);
    this->setWindowFlags(Qt::Window);

    // Construct the status bar.
    {
        auto *const bar = new QStatusBar();
        this->layout()->addWidget(bar);

        auto *const viewScale = new QLabel("Scale: 1.0");
        connect(ui->graphicsView, &InteractibleNodeGraphView::scale_changed, this, [=](const double newScale)
        {
            viewScale->setText(QString("Scale: %1").arg(QString::number(newScale, 'f', 1)));
        });

        bar->addPermanentWidget(viewScale);
    }

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Graph...
        {
            QMenu *filterGraphMenu = new QMenu("Graph", this->menubar);

            QAction *enable = new QAction("Enabled", this->menubar);
            enable->setCheckable(true);
            enable->setChecked(this->isEnabled);

            connect(this, &FilterGraphDialog::filter_graph_enabled, this, [=]
            {
                enable->setChecked(true);
            });

            connect(this, &FilterGraphDialog::filter_graph_disabled, this, [=]
            {
                enable->setChecked(false);
            });

            connect(enable, &QAction::triggered, this, [=]
            {
                this->set_filter_graph_enabled(!this->isEnabled);
            });

            filterGraphMenu->addAction(enable);

            filterGraphMenu->addSeparator();
            connect(filterGraphMenu->addAction("New"), &QAction::triggered, this, [=]{this->reset_graph();});
            filterGraphMenu->addSeparator();
            connect(filterGraphMenu->addAction("Load..."), &QAction::triggered, this, [=]{this->load_filters();});
            connect(filterGraphMenu->addAction("Save as..."), &QAction::triggered, this, [=]{this->save_filters();});

            this->menubar->addMenu(filterGraphMenu);
        }

        // Nodes...
        {
            QMenu *filtersMenu = new QMenu("Filters", this);

            // Insert the names of all available filter types.
            {
                QMenu *enhanceMenu = new QMenu("Enhance", this);
                QMenu *reduceMenu = new QMenu("Reduce", this);
                QMenu *distortMenu = new QMenu("Distort", this);
                QMenu *metaMenu = new QMenu("Information", this);

                std::vector<const filter_c::filter_metadata_s*> knownFilterTypes = kf_known_filter_types();

                std::sort(knownFilterTypes.begin(), knownFilterTypes.end(), [](const filter_c::filter_metadata_s *a, const filter_c::filter_metadata_s *b)
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

                    connect(filtersMenu->addAction(QString::fromStdString(filter->name)), &QAction::triggered, this,
                            [=]{this->add_filter_node(filter->type);});
                }

                filtersMenu->addSeparator();

                // Add filters.
                {
                    filtersMenu->addMenu(enhanceMenu);
                    filtersMenu->addMenu(reduceMenu);
                    filtersMenu->addMenu(distortMenu);
                    filtersMenu->addMenu(metaMenu);

                    for (const auto filter: knownFilterTypes)
                    {
                        if ((filter->type == filter_type_enum_e::output_gate) ||
                            (filter->type == filter_type_enum_e::input_gate))
                        {
                            continue;
                        }

                        auto *const action = new QAction(QString::fromStdString(filter->name));

                        switch (filter->category)
                        {
                            case filter_category_e::distort: distortMenu->addAction(action); break;
                            case filter_category_e::enhance: enhanceMenu->addAction(action); break;
                            case filter_category_e::reduce: reduceMenu->addAction(action); break;
                            case filter_category_e::meta: metaMenu->addAction(action); break;
                            default: k_assert(0, "Unknown filter category."); break;
                        }

                        connect(action, &QAction::triggered, this,
                                [=]{this->add_filter_node(filter->type);});
                    }
                }
            }

            this->menubar->addMenu(filtersMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Create and configure the graphics scene.
    {
        this->graphicsScene = new InteractibleNodeGraph(this);
        this->graphicsScene->setBackgroundBrush(QBrush("transparent"));

        ui->graphicsView->setScene(this->graphicsScene);

        connect(this->graphicsScene, &InteractibleNodeGraph::edgeConnectionAdded, this, [this]{this->recalculate_filter_chains();});
        connect(this->graphicsScene, &InteractibleNodeGraph::edgeConnectionRemoved, this, [this]{this->recalculate_filter_chains();});
        connect(this->graphicsScene, &InteractibleNodeGraph::nodeRemoved, this, [this](InteractibleNodeGraphNode *const node)
        {
            FilterGraphNode *const filterNode = dynamic_cast<FilterGraphNode*>(node);

            if (filterNode)
            {
                if (filterNode->associatedFilter->metaData.type == filter_type_enum_e::input_gate)
                {
                    this->inputGateNodes.erase(std::find(inputGateNodes.begin(), inputGateNodes.end(), filterNode));
                }

                delete filterNode;

                /// TODO: When a node is deleted, recalculate_filter_chains() gets
                /// called quite a few times more than needed - once for each of its
                /// connections, and a last time here.
                this->recalculate_filter_chains();
            }
        });
    }

    // Restore persistent settings.
    {
        this->set_filter_graph_enabled(kpers_value_of(INI_GROUP_OUTPUT, "custom_filtering", kf_is_filtering_enabled()).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "filter_graph", this->size()).toSize());
    }

    this->reset_graph(true);

    return;
}

FilterGraphDialog::~FilterGraphDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OUTPUT, "custom_filtering", this->isEnabled);
        kpers_set_value(INI_GROUP_GEOMETRY, "filter_graph", this->size());
    }

    delete ui;

    return;
}

void FilterGraphDialog::reset_graph(const bool autoAccept)
{
    if (autoAccept ||
        QMessageBox::question(this,
                              "Create a new graph?",
                              "Any unsaved changes in the current graph will be lost. Continue?") == QMessageBox::Yes)
    {
        this->clear_filter_graph();
    }

    return;
}

void FilterGraphDialog::load_filters(void)
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Select a file containing the filter graph to be loaded", "",
                                                    "Filter graphs (*.vcs-filter-graph);;"
                                                    "All files(*.*)");

    if (filename.isEmpty())
    {
        return;
    }

    const auto filterData = kdisk_load_filter_graph(filename.toStdString());

    if (!filterData.first.empty())
    {
        this->set_filter_graph_source_filename(filename);
        this->set_filter_graph_options(filterData.second);
    }

    return;
}

void FilterGraphDialog::save_filters(void)
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Select a file to save the filter graph into", "",
                                                    "Filter files (*.vcs-filter-graph);;"
                                                    "All files(*.*)");

    if (filename.isEmpty())
    {
        return;
    }

    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".vcs-filter-graph";
    }

    std::vector<FilterGraphNode*> filterNodes;
    {
        for (auto node: this->graphicsScene->items())
        {
            const auto filterNode = dynamic_cast<FilterGraphNode*>(node);

            if (filterNode)
            {
                filterNodes.push_back(filterNode);
            }
        }
    }

    std::vector<filter_graph_option_s> graphOptions;
    {
        /// TODO.
    }

    if (kdisk_save_filter_graph(filterNodes, graphOptions, filename.toStdString()))
    {
        this->set_filter_graph_source_filename(filename);
    }

    return;
}

// Adds a new instance of the given filter type into the node graph. Returns a
// pointer to the new node.
FilterGraphNode* FilterGraphDialog::add_filter_node(const filter_type_enum_e type,
                                                    const u8 *const initialParameterValues)
{
    const filter_c *const newFilter = kf_create_new_filter_instance(type, initialParameterValues);
    const unsigned filterWidgetWidth = (newFilter->guiWidget->widget->width() + 20);
    const unsigned filterWidgetHeight = (newFilter->guiWidget->widget->height() + 35);
    const QString nodeTitle = QString("%1. %2").arg(this->numNodesAdded+1).arg(newFilter->guiWidget->title);

    k_assert(newFilter, "Failed to add a new filter node.");

    FilterGraphNode *newNode = nullptr;

    switch (type)
    {
        case filter_type_enum_e::input_gate: newNode = new InputGateNode(nodeTitle, filterWidgetWidth, filterWidgetHeight); break;
        case filter_type_enum_e::output_gate: newNode = new OutputGateNode(nodeTitle, filterWidgetWidth, filterWidgetHeight); break;
        default: newNode = new FilterNode(nodeTitle, filterWidgetWidth, filterWidgetHeight); break;
    }

    newNode->associatedFilter = newFilter;
    newNode->setFlags(newNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges | QGraphicsItem::ItemIsSelectable);
    this->graphicsScene->addItem(newNode);

    QGraphicsProxyWidget* nodeWidgetProxy = new QGraphicsProxyWidget(newNode);
    nodeWidgetProxy->setWidget(newFilter->guiWidget->widget);
    nodeWidgetProxy->widget()->move(10, 30);

    if (type == filter_type_enum_e::input_gate)
    {
        this->inputGateNodes.push_back(newNode);
    }

    newNode->moveBy(rand()%20, rand()%20);

    real maxZ = 0;
    const auto sceneItems = this->graphicsScene->items();
    for (auto item: sceneItems)
    {
        if (item->zValue() > maxZ)
        {
            maxZ = item->zValue();
        }
    }
    newNode->setZValue(maxZ+1);

    ui->graphicsView->centerOn(newNode);

    this->numNodesAdded++;

    return newNode;
}

// Visit each node in the graph and while doing so, group together such chains of
// filters that run from an input gate through one or more filters into an output
// gate. The chains will then be submitted to the filter handler for use in applying
// the filters to captured frames.
void FilterGraphDialog::recalculate_filter_chains(void)
{
    kf_remove_all_filter_chains();

    const std::function<void(FilterGraphNode *const, std::vector<const filter_c*>)> traverse_filter_node =
          [&](FilterGraphNode *const node, std::vector<const filter_c*> accumulatedFilterChain)
    {
        k_assert((node && node->associatedFilter), "Trying to visit an invalid node.");

        if (std::find(accumulatedFilterChain.begin(),
                      accumulatedFilterChain.end(),
                      node->associatedFilter) != accumulatedFilterChain.end())
        {
            kd_show_headless_error_message("VCS detected a potential infinite loop",
                                           "One or more filter chains in the filter graph are connected in a loop "
                                           "(a node's output connects back to its input).\n\nChains containing an "
                                           "infinite loop will remain unusable until the loop is disconnected.");

            return;
        }

        if (node->is_enabled())
        {
            accumulatedFilterChain.push_back(node->associatedFilter);
        }

        if (node->associatedFilter->metaData.type == filter_type_enum_e::output_gate)
        {
            kf_add_filter_chain(accumulatedFilterChain);
            return;
        }

        // NOTE: This assumes that each node in the graph only has one output edge.
        for (auto outgoing: node->output_edge()->connectedTo)
        {
            traverse_filter_node(dynamic_cast<FilterGraphNode*>(outgoing->parentNode), accumulatedFilterChain);
        }

        return;
    };

    for (auto inputGate: this->inputGateNodes)
    {
        traverse_filter_node(inputGate, {});
    }

    return;
}

void FilterGraphDialog::clear_filter_graph(void)
{
    kf_remove_all_filter_chains();
    this->graphicsScene->reset_scene();
    this->inputGateNodes.clear();
    this->numNodesAdded = 0;

    this->setWindowTitle(QString("%1 - %2").arg(this->dialogBaseTitle).arg("Unsaved graph"));

    return;
}

void FilterGraphDialog::refresh_filter_graph(void)
{
    this->graphicsScene->update_scene_connections();

    return;
}

void FilterGraphDialog::set_filter_graph_source_filename(const QString &sourceFilename)
{
    const QString baseFilename = QFileInfo(sourceFilename).baseName();

    this->setWindowTitle(QString("%1 - %2").arg(this->dialogBaseTitle).arg(baseFilename));

    // Kludge fix for the filter graph not repainting itself properly when new nodes
    // are loaded in. Let's just force it to do so.
    this->refresh_filter_graph();

    return;
}

void FilterGraphDialog::set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions)
{
    for (const filter_graph_option_s &option: graphOptions)
    {
        /// TODO.

        (void)option;
    }

    return;
}

void FilterGraphDialog::set_filter_graph_enabled(const bool enabled)
{
    this->isEnabled = enabled;

    if (!this->isEnabled)
    {
        emit this->filter_graph_disabled();
    }
    else
    {
        emit this->filter_graph_enabled();
    }

    kf_set_filtering_enabled(this->isEnabled);

    kd_update_output_window_title();

    return;
}

bool FilterGraphDialog::is_filter_graph_enabled(void)
{
    return this->isEnabled;
}


FilterGraphNode* FilterGraphDialog::add_filter_graph_node(const filter_type_enum_e &filterType,
                                                          const u8 *const initialParameterValues)
{
    return add_filter_node(filterType, initialParameterValues);
}
