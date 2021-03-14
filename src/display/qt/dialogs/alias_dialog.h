/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_ALIAS_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ALIAS_DIALOG_H

#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class QMenuBar;

namespace Ui {
class AliasDialog;
}

struct mode_alias_s;

class AliasDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit AliasDialog(QWidget *parent = 0);

    ~AliasDialog(void);

    void clear_known_aliases(void);

    void add_alias_to_list(const mode_alias_s a);

    void load_aliases_from_file(const QString &filename);

private:
    Ui::AliasDialog *ui;

    void save_aliases_to_file(QString filename);

    void remove_selected_aliases(void);

    void remove_all_aliases(void);

    void add_new_alias(void);

    void broadcast_aliases();

    void adjust_treewidget_column_size();

    void resizeEvent(QResizeEvent *);

    void showEvent(QShowEvent *event);

    void assign_aliases(const std::vector<mode_alias_s> &aliases);

};

#endif
