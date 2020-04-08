#ifndef VIDEO_PARAMETER_GRAPH_DIALOG_H
#define VIDEO_PARAMETER_GRAPH_DIALOG_H

#include <QDialog>
#include "filter/filter.h"

class InteractibleNodeGraph;
class QMenuBar;

namespace Ui {
class VideoParameterDialog;
}

class VideoParameterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoParameterDialog(QWidget *parent = 0);
    ~VideoParameterDialog();

    void set_video_parameter_graph_enabled(const bool enabled);

    bool is_video_parameter_graph_enabled(void);

signals:
    // Emitted when the graph's enabled status is toggled.
    void video_parameter_graph_enabled(void);
    void video_parameter_graph_disabled(void);

private:
    // Whether the filter graph is enabled.
    bool isEnabled = false;

    Ui::VideoParameterDialog *ui;

    QMenuBar *menubar = nullptr;
    InteractibleNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    unsigned numNodesAdded = 0;

    // The dialog's title, without any additional information that may be appended,
    // like the name of the file from which the dialog's current data was loaded.
    const QString dialogBaseTitle = "VCS - Video Parameter Graph";
};

#endif
