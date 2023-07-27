#ifndef VCS_DISPLAY_QT_DIALOGS_FILTER_GRAPH_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_FILTER_GRAPH_DIALOG_H

#include "display/qt/widgets/QDialog_vcs_dialog_fragment.h"
#include "filter/filter.h"

class InteractibleNodeGraph;
class BaseFilterGraphNode;
class QMenuBar;
class QMenu;

namespace control_panel
{
    namespace Ui { class FilterGraph; }

    class FilterGraph : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit FilterGraph(QWidget *parent = 0);
        ~FilterGraph();

        void recalculate_filter_chains(void);

        void clear_filter_graph(void);

        void refresh_filter_graph(void);

        // Loads the filter graph from the most recent used graph file. Returns true
        // if the graph was loaded; false otherwise.
        bool restore_persistent_graph(void);

        bool load_graph_from_file(const QString &filename);

    private:
        BaseFilterGraphNode* add_filter_graph_node(
            const std::string &filterTypeUuid,
            const filter_params_t &initialParamValues = {}
        );

        void reset_graph(const bool autoAccept = false);

        void save_graph_into_file(QString filename);

        Ui::FilterGraph *ui;

        InteractibleNodeGraph *graphicsScene = nullptr;

        // Contains actions corresponding to all the filters that can be added to the
        // filter graph.
        QMenu *filtersMenu = nullptr;

        // All the nodes that are currently in the graph.
        std::vector<BaseFilterGraphNode*> inputGateNodes;

        unsigned numNodesAdded = 0;
    };
}

#endif
