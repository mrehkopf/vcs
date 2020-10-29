#ifndef VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H

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

    // Called to inform the dialog of a new source file for video presets.
    void assign_presets(const std::vector<video_preset_s*> &presets);

signals:
    // Emitted when the last item in the preset list is removed.
    void preset_list_became_empty(void);

    // Emitted when an item is added to an empty preset list.
    void preset_list_no_longer_empty(void);

private:
    void add_video_preset_to_list(const video_preset_s *const preset);

    void update_preset_control_ranges(void);

    void update_preset_controls_with_current_preset_data(void);

    QString make_preset_list_text(const video_preset_s *const preset);

    void broadcast_current_preset_parameters(void);

    void update_current_present_list_text(void);

    void resort_preset_list(void);

    void remove_video_preset_from_list(const video_preset_s *const preset);

    void remove_all_video_presets_from_list(void);

    void set_video_params_source_filename(const QString &filename);

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
