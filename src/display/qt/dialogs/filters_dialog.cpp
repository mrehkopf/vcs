#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QTimer>
#include "display/qt/subclasses/QGraphicsScene_forward_node_graph.h"
#include "display/qt/dialogs/filters_dialog_nodes.h"
#include "display/qt/dialogs/filters_dialog.h"
#include "display/qt/widgets/filter_widgets.h"
#include "ui_filters_dialog.h"
#include "filter/filter.h"

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

        // Nodes...
        {
            QMenu *fileMenu = new QMenu("Nodes", this);

            fileMenu->addAction("Add");

            menubar->addMenu(fileMenu);
        }

        // Help...
        {
            QMenu *fileMenu = new QMenu("Help", this);

            fileMenu->addAction("About...");
            fileMenu->actions().at(0)->setEnabled(false); /// TODO: Add the actual help.

            menubar->addMenu(fileMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Create and configure the graphics scene.
    {
        this->graphicsScene = new ForwardNodeGraph(this);

        this->graphicsScene->setBackgroundBrush(QBrush("#303030"));

        ui->graphicsView->setScene(this->graphicsScene);
    }

    // For temporary testing purposes, add some placeholder nodes into the graphics scene.
    {
        const filter_c *const blurFilter = kf_create_filter("a5426f2e-b060-48a9-adf8-1646a2d3bd41");
        const filter_c *const rotateFilter = kf_create_filter("140c514d-a4b0-4882-abc6-b4e9e1ff4451");
        const filter_c *const inputGateFilter = kf_create_filter("136deb34-ac79-46b1-a09c-d57dcfaa84ad");

        FilterNode *filterNode = new FilterNode(blurFilter->guiWidget->title);
        filterNode->setFlags(filterNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode);

        FilterNode *filterNode2 = new FilterNode(rotateFilter->guiWidget->title);
        filterNode2->setFlags(filterNode2->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode2);

        InputGateNode *inputNode = new InputGateNode(inputGateFilter->guiWidget->title);
        inputNode->setFlags(inputNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(inputNode);

        QGraphicsProxyWidget* filterNodeProxy = new QGraphicsProxyWidget(filterNode);
        filterNodeProxy->setWidget(blurFilter->guiWidget->widget);
        filterNodeProxy->widget()->move(10, 45); // Center widget in the graphics node.

        QGraphicsProxyWidget* filterNodeProxy2 = new QGraphicsProxyWidget(filterNode2);
        filterNodeProxy2->setWidget(rotateFilter->guiWidget->widget);
        filterNodeProxy2->widget()->move(10, 45);

        QGraphicsProxyWidget* filterNodeProxy3 = new QGraphicsProxyWidget(inputNode);
        filterNodeProxy3->setWidget(inputGateFilter->guiWidget->widget);
        filterNodeProxy3->widget()->move(10, 45);

        filterNode2->moveBy(350, 0);
        inputNode->moveBy(-350, 0);
    }

    return;
}

FiltersDialog::~FiltersDialog()
{
    delete ui;

    return;
}
