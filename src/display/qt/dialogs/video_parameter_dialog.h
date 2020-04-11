#ifndef VIDEO_PARAMETER_GRAPH_DIALOG_H
#define VIDEO_PARAMETER_GRAPH_DIALOG_H

#include <QDialog>
#include "filter/filter.h"
#include "display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h"

class InteractibleNodeGraph;
class VideoGraphNode;
class QMenuBar;

struct video_preset_s;

enum class video_graph_node_type_e
{
    video_parameters,
    resolution,
    label,
    refresh_rate,
    shortcut
};

class VideoGraphNode : public QObject, public InteractibleNodeGraphNode
{
    Q_OBJECT

public:
};

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

    // Called to inform the dialog of a new source file for video presets.
    void receive_new_video_presets_filename(const QString &filename);

    // Called to inform the dialog that a new capture signal has been received.
    void notify_of_new_capture_signal(void);

signals:
    // Emitted when the graph's enabled status is toggled.
    void video_parameter_graph_enabled(void);
    void video_parameter_graph_disabled(void);

private:
    void add_video_preset_to_list(const video_preset_s *const preset);

    void update_preset_control_ranges(void);

    void update_preset_controls(void);

    QString make_preset_list_text(const video_preset_s *const preset);

    void broadcast_current_preset_parameters(void);

    void update_current_present_list_text(void);

    void resort_preset_list(void);

    void remove_video_preset_from_list(const video_preset_s *const preset);

    void remove_all_video_presets_from_list(void);


    // Whether the filter graph is enabled.
    bool isEnabled = false;

    Ui::VideoParameterDialog *ui;

    QMenuBar *menubar = nullptr;
    InteractibleNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    // The video preset we're currently editing.
    video_preset_s *currentPreset = nullptr;

    unsigned numNodesAdded = 0;

    // The dialog's title, without any additional information that may be appended,
    // like the name of the file from which the dialog's current data was loaded.
    const QString dialogBaseTitle = "VCS - Video Presets";

};

#endif
