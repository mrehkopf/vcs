#include <QMenuBar>
#include "display/qt/subclasses/QGraphicsScene_forward_node_graph.h"
#include "display/qt/dialogs/filters_dialog_nodes.h"
#include "display/qt/dialogs/filters_dialog.h"
#include "ui_filters_dialog.h"

FiltersDialog::FiltersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FiltersDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Filters");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

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

    // Add some placeholder nodes into the graphics scene, for testing purposes.
    {
        FilterNode *filterNode = new FilterNode("Blur");
        filterNode->setFlags(filterNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode);

        FilterNode *filterNode2 = new FilterNode("Sharpen");
        filterNode2->setFlags(filterNode2->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode2);

        filterNode2->moveBy(350, 0);
    }

    return;
}

FiltersDialog::~FiltersDialog()
{
    delete ui;

    return;
}
