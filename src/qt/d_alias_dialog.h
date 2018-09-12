/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_ALIAS_DIALOG_H
#define D_ALIAS_DIALOG_H

#include <QDialog>

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
    void on_pushButton_load_clicked();

private:
    Ui::AliasDialog *ui;
};

#endif // D_ALIAS_DIALOG_H
