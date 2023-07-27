/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H
#define VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H

#include <QMenu>
#include "display/qt/widgets/QDialog_vcs_dialog_fragment.h"

class DialogFileMenu : public QMenu
{
    Q_OBJECT

public:
    explicit DialogFileMenu(DialogFragment *const parentDialog);
    
    ~DialogFileMenu(void);

signals:
    // Emitted when the corresponding actions are triggered.
    void save_as(const QString &originalFilename);
    void save(const QString &filename);
    void open(void);
    void close(const QString &filename);

private:
    DialogFragment *const parentDialog;
};

#endif
