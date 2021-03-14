/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H
#define VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H

#include <QMenu>
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

class DialogFileMenu : public QMenu
{
    Q_OBJECT

public:
    explicit DialogFileMenu(VCSBaseDialog *const parentDialog);
    
    ~DialogFileMenu(void);

signals:
    // Emitted when the corresponding actions are triggered.
    void save_as(const QString &originalFilename);
    void save(const QString &filename);
    void open(void);
    void close(const QString &filename);

private:
    VCSBaseDialog *const parentDialog;
};

#endif
