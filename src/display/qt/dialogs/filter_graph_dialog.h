#ifndef FILTER_GRAPH_DIALOG_H_
#define FILTER_GRAPH_DIALOG_H_

#include <QDialog>
#include "filter/filter.h"

class InteractibleNodeGraph;
class FilterGraphNode;
class QMenuBar;

namespace Ui {
class FilterGraphDialog;
}

class FilterGraphDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterGraphDialog(QWidget *parent = 0);
    ~FilterGraphDialog();

    void recalculate_filter_chains(void);

    void clear_filter_graph(void);

    FilterGraphNode* add_filter_graph_node(const filter_type_enum_e &filterType, const u8 * const initialParameterValues);

    FilterGraphNode* add_filter_node(const filter_type_enum_e type, const u8 *const initialParameterValues = nullptr);

    void set_filter_graph_source_filename(const std::string &sourceFilename);

    void refresh_filter_graph(void);

private:
    void reset_graph(const bool autoAccept = false);
    void save_filters(void);
    void load_filters(void);

    Ui::FilterGraphDialog *ui;
    QMenuBar *menubar = nullptr;
    InteractibleNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    unsigned numNodesAdded = 0;

    // The dialog's title, without any additional information that may be appended,
    // like the name of the file from which the dialog's current data was loaded.
    const QString dialogBaseTitle = "VCS - Filter Graph";
};

#endif
