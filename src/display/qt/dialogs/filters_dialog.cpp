#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QTimer>
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

    // For temporary testing purposes, add some placeholder nodes into the graphics scene.
    {
        u8 *const blurFilterData = new u8[FILTER_DATA_LENGTH];
        u8 *const rotateFilterData = new u8[FILTER_DATA_LENGTH];

        const filter_widget_blur_s *const blurFilterWidget = new filter_widget_blur_s(blurFilterData);
        const filter_widget_rotate_s *const rotateFilterWidget = new filter_widget_rotate_s(rotateFilterData);

        FilterNode *filterNode = new FilterNode(QString::fromStdString(blurFilterWidget->name));
        filterNode->setFlags(filterNode->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode);

        FilterNode *filterNode2 = new FilterNode(QString::fromStdString(rotateFilterWidget->name));
        filterNode2->setFlags(filterNode2->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
        this->graphicsScene->addItem(filterNode2);

        QGraphicsProxyWidget* filterNodeProxy = new QGraphicsProxyWidget(filterNode);
        filterNodeProxy->setWidget(blurFilterWidget->widget);
        filterNodeProxy->widget()->move(10, 45); // Center widget in the graphics node.

        QGraphicsProxyWidget* filterNodeProxy2 = new QGraphicsProxyWidget(filterNode2);
        filterNodeProxy2->setWidget(rotateFilterWidget->widget);
        filterNodeProxy2->widget()->move(10, 45);

        filterNode2->moveBy(350, 0);

        QTimer *t = new QTimer;
        connect(t, &QTimer::timeout, [=]
        {
            QString bytes;

            for (unsigned i = 0; i < FILTER_DATA_LENGTH; i++)
            {
                bytes += QString("%1 ").arg(QString::number(rotateFilterData[i]));
            }

            qDebug() << bytes;
        });
        t->start(1000);
    }

    return;
}

FiltersDialog::~FiltersDialog()
{
    delete ui;

    return;
}
