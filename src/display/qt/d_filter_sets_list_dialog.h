/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_FILTER_DIALOG_H
#define D_FILTER_DIALOG_H

#include <QDialog>
#include "../../common/types.h"

class QTreeWidgetItem;
class QListWidgetItem;
class QMenuBar;
struct capture_signal_s;
struct filter_set_s;

namespace Ui {
class FilterSetsListDialog;
}

class FilterSetsListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterSetsListDialog(QWidget *parent = 0);
    ~FilterSetsListDialog();

    void repopulate_filter_sets_list(const int newIdx = -1);

    void receive_current_filter_set_idx(const int idx);

    void signal_filtering_enabled(const bool state);

    void update_filter_sets_list();

private slots:
    void on_treeWidget_setList_itemChanged(QTreeWidgetItem *item);

    void add_new_set();

    void on_treeWidget_setList_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_pushButton_remove_clicked();

    void on_pushButton_up_clicked();

    void on_pushButton_down_clicked();

    bool load_sets_from_file();

    bool save_sets_to_file();

    void on_pushButton_add_clicked();

private:
    void create_menu_bar();

    void flag_unsaved_changes();

    Ui::FilterSetsListDialog *ui;

    QMenuBar *menubar = nullptr;

    // Reflects whether the user has enabled or disabled custom filtering in the
    // control panel.
    bool filteringIsEnabled = false;
};

#endif // D_FILTER_DIALOG_H
