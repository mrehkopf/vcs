/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_DIALOGS_ANTI_TEAR_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_ANTI_TEAR_DIALOG_H

#include <QDialog>

class QMenuBar;

namespace Ui {
class AntiTearDialog;
}

class AntiTearDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AntiTearDialog(QWidget *parent = 0);

    ~AntiTearDialog();

    bool is_anti_tear_enabled(void);

    void set_anti_tear_enabled(const bool enabled);

signals:
    // Emitted when the anti-tear's enabled status is toggled.
    void anti_tear_enabled(void);
    void anti_tear_disabled(void);

private:
    void update_visualization_options(void);

    Ui::AntiTearDialog *ui;

    // Whether anti-tearing is enabled.
    bool isEnabled = false;

    QMenuBar *menubar = nullptr;
};

#endif
