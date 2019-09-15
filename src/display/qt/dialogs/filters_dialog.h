#ifndef FILTERS_DIALOG_H
#define FILTERS_DIALOG_H

#include <QDialog>
#include "filter/filter.h"

class ForwardNodeGraph;
class FilterGraphNode;
class QMenuBar;

namespace Ui {
class FiltersDialog;
}

class FiltersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FiltersDialog(QWidget *parent = 0);
    ~FiltersDialog();

    void recalculate_filter_chains(void);

public slots:
    FilterGraphNode* add_filter_node(const filter_type_enum_e type);

private:
    Ui::FiltersDialog *ui;
    QMenuBar *menubar = nullptr;
    ForwardNodeGraph *graphicsScene = nullptr;

    // All the nodes that are currently in the graph.
    std::vector<FilterGraphNode*> inputGateNodes;

    unsigned numNodesAdded = 0;
};

#endif
