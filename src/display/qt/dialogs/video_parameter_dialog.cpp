#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QTimer>
#include <functional>
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/disk/disk.h"
#include "ui_video_parameter_dialog.h"

VideoParameterDialog::VideoParameterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoParameterDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(this->dialogBaseTitle);
    this->setWindowFlags(Qt::Window);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Filter graph...
        {
            QMenu *filterGraphMenu = new QMenu("Graph", this->menubar);

            QAction *enable = new QAction("Enabled", this->menubar);
            enable->setCheckable(true);
            enable->setChecked(this->isEnabled);

            filterGraphMenu->addAction(enable);

            filterGraphMenu->addSeparator();
            connect(filterGraphMenu->addAction("New graph"), &QAction::triggered, this, [=]{});
            filterGraphMenu->addSeparator();
            connect(filterGraphMenu->addAction("Load graph..."), &QAction::triggered, this, [=]{});
            filterGraphMenu->addSeparator();
            connect(filterGraphMenu->addAction("Save graph..."), &QAction::triggered, this, [=]{});

            this->menubar->addMenu(filterGraphMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Create and configure the graphics scene.
    {
        this->graphicsScene = new InteractibleNodeGraph(this);
        this->graphicsScene->setBackgroundBrush(QBrush("gray"));

        ui->graphicsView->setScene(this->graphicsScene);

        connect(this->graphicsScene, &InteractibleNodeGraph::edgeConnectionAdded, this, [this]{});
        connect(this->graphicsScene, &InteractibleNodeGraph::edgeConnectionRemoved, this, [this]{});
        connect(this->graphicsScene, &InteractibleNodeGraph::nodeRemoved, this, [this](InteractibleNodeGraphNode *const node){});
    }

    // Restore persistent settings.
    {
       // this->set_filter_graph_enabled(kpers_value_of(INI_GROUP_OUTPUT, "custom_filtering", kf_is_filtering_enabled()).toBool());
       // this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "filter_graph", this->size()).toSize());
    }

    return;
}

VideoParameterDialog::~VideoParameterDialog()
{
    // Save persistent settings.
    {
       // kpers_set_value(INI_GROUP_OUTPUT, "custom_filtering", this->isEnabled);
      //  kpers_set_value(INI_GROUP_GEOMETRY, "filter_graph", this->size());
    }

    delete ui;

    return;
}

void VideoParameterDialog::set_video_parameter_graph_enabled(const bool enabled)
{
    this->isEnabled = enabled;

    if (!this->isEnabled)
    {
        emit this->video_parameter_graph_disabled();
    }
    else
    {
        emit this->video_parameter_graph_enabled();
    }

    kf_set_filtering_enabled(this->isEnabled);

    kd_update_output_window_title();

    return;
}

bool VideoParameterDialog::is_video_parameter_graph_enabled(void)
{
    DEBUG(("WSD"));
    return this->isEnabled;
}
