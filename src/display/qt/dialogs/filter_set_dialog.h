#ifndef FILTER_SET_DIALOG_H
#define FILTER_SET_DIALOG_H

#include <QDialog>
#include "filter/filter.h"

namespace Ui {
class FilterSetDialog;
}

struct filter_set_s;

class FilterSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterSetDialog(filter_set_s *const filterSet, QWidget *parent = 0, const bool allowApplyButton = true);

    ~FilterSetDialog();

private:
    Ui::FilterSetDialog *ui;

    // A pointer to the filter set that we'll modify.
    filter_set_s *const filterSet;

    // A copy of the original filter set, for canceling any edit we've made.
    filter_set_s originalFilterSet;
};

#endif
