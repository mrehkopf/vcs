/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_ALIAS_DIALOG_H
#define D_ALIAS_DIALOG_H

#include <QDialog>

class QMenuBar;
class QTreeWidgetItem;
struct mode_alias_s;

namespace Ui {
class AliasDialog;
}

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

    void on_pushButton_removeAlias_clicked();

    void on_pushButton_addAlias_clicked();

private:
    Ui::AliasDialog *ui;

    void create_menu_bar();

    void broadcast_aliases();

    QWidget* create_resolution_widget(QTreeWidgetItem *parentItem, const uint column, const uint width, const uint height);

    void adjust_tree_widget_column_size();

    void resizeEvent(QResizeEvent *);

    QMenuBar *menubar = nullptr;
};

#endif
