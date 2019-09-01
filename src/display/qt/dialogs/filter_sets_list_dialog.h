/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef FILTER_SETS_LIST_DIALOG_H
#define FILTER_SETS_LIST_DIALOG_H

#include <QDialog>
#include "common/types.h"

class QTreeWidgetItem;
class QListWidgetItem;
class QMenuBar;

namespace Ui {
class FilterSetsListDialog;
}

struct capture_signal_s;
struct filter_set_s;

class FilterSetsListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterSetsListDialog(QWidget *parent = 0);
    ~FilterSetsListDialog();

    void update_filter_set_idx(void);

    void signal_filtering_enabled(const bool state);

    void update_filter_sets_list(void);

private slots:
    bool load_sets_from_file(void);

    bool save_sets_to_file(void);

private:
    void flag_unsaved_changes(void);

    void repopulate_filter_sets_list(int newIdx = -1);

    void remove_unsaved_changes_flag();

    void edit_filter_set(const QTreeWidgetItem *const item);

    void update_selection_sensitive_controls(void);

    Ui::FilterSetsListDialog *ui;

    QMenuBar *menubar = nullptr;

    // Reflects whether the user has enabled or disabled custom filtering in the
    // control panel.
    bool filteringIsEnabled = false;
};

#endif
