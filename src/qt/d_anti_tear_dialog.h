/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef D_ANTI_TEAR_DIALOG_H
#define D_ANTI_TEAR_DIALOG_H

#include <QDialog>

namespace Ui {
class AntiTearDialog;
}

class AntiTearDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AntiTearDialog(QWidget *parent = 0);
    ~AntiTearDialog();

private slots:
    void on_spinBox_rangeUp_valueChanged(int);

    void on_spinBox_rangeDown_valueChanged(int);

    void on_spinBox_domainSize_valueChanged(int);

    void on_spinBox_matchedReqd_valueChanged(int);

    void on_groupBox_visualization_toggled(bool);

    void on_checkBox_visualizeRange_stateChanged(int);

    void on_checkBox_visualizeTear_stateChanged(int);

    void on_pushButton_load_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_restoreDefaults_clicked();

    void on_spinBox_stepSize_valueChanged(int arg1);

    void on_spinBox_threshold_valueChanged(int arg1);

private:
    void update_visualization_options();

    void load_params(const QString filename);

    void save_params(const QString filename);

    Ui::AntiTearDialog *ui;
};

#endif // D_ANTI_TEAR_DIALOG_H
