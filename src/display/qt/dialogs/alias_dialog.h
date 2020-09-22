/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_ALIAS_DIALOG_H
#define D_ALIAS_DIALOG_H

#include <QDialog>

class QMenuBar;

namespace Ui {
class AliasDialog;
}

struct mode_alias_s;

class AliasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AliasDialog(QWidget *parent = 0);

    ~AliasDialog();

    void clear_known_aliases();

    void receive_new_alias(const mode_alias_s a);

private slots:
    void load_aliases();

    void save_aliases();

    void remove_selected_aliases();

    void add_alias();

private:
    Ui::AliasDialog *ui;

    void broadcast_aliases();

    void adjust_treewidget_column_size();

    void resizeEvent(QResizeEvent *);

    void showEvent(QShowEvent *event);

    QMenuBar *menubar = nullptr;
};

#endif
