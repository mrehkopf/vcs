/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H
#define VCS_DISPLAY_QT_SUBCLASSES_QMENU_DIALOG_FILE_MENU_H

#include <QMenu>

class DialogFileMenu : public QMenu
{
    Q_OBJECT

public:
    explicit DialogFileMenu(QWidget *parent = 0);
    
    ~DialogFileMenu();

    void report_new_source_file(const QString &newFilename);

signals:
    // Gets emitted when this->filename is changed.
    void new_source_file(const QString &filename);

    // Emitted when the corresponding actions are triggered.
    void user_wants_to_save_as(const QString &originalFilename);
    void user_wants_to_save(const QString &filename);
    void user_wants_to_open(void);
    void user_wants_to_close(const QString &filename);

private:
};

#endif
