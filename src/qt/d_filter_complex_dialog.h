/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_FILTER_DIALOG_H
#define D_FILTER_DIALOG_H

#include <QDialog>
#include "../types.h"

struct filter_complex_s;

class QListWidgetItem;

namespace Ui {
class FilterDialog;
}

class FilterComplexDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilterComplexDialog(QWidget *parent = 0);
    ~FilterComplexDialog();

private slots:
    void on_listWidget_complexList_itemChanged(QListWidgetItem *item);

    void on_listWidget_preFilters_itemChanged(QListWidgetItem *item);

    void on_listWidget_postFilters_itemChanged(QListWidgetItem *item);

    void on_listWidget_preFilters_itemDoubleClicked(QListWidgetItem *item);

    void on_listWidget_postFilters_itemDoubleClicked(QListWidgetItem *item);

    void on_pushButton_updateComplex_clicked();

    void on_listWidget_complexList_itemSelectionChanged();

    void on_pushButton_saveAll_clicked();

    void on_pushButton_loadAll_clicked();

private:
    void populate_scaling_filter_combobox();

    void update_resolution_spinbox_limits();

    void populate_filter_list();

    filter_complex_s make_current_filter_complex();

    bool load_complexes_from_file(const QString &filename, const bool automatic);

    void assign_data_block(QListWidgetItem * const item, const QString &filterType);

    Ui::FilterDialog *ui;
};

#endif // D_FILTER_DIALOG_H
