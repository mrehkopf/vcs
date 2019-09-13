#ifndef FILTERS_DIALOG_H
#define FILTERS_DIALOG_H

#include <QDialog>
#include "filter/filter.h"

class ForwardNodeGraphNode;
class QGraphicsScene;
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

private:
    Ui::FiltersDialog *ui;

    QMenuBar *menubar = nullptr;
    QGraphicsScene *graphicsScene = nullptr;

public slots:
    ForwardNodeGraphNode* add_filter_node(const filter_enum_e type);
};

#endif
