/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QDIALOG_VCS_DIALOG_FRAGMENT_H
#define VCS_DISPLAY_QT_SUBCLASSES_QDIALOG_VCS_DIALOG_FRAGMENT_H

#include <QWidget>

class QMenuBar;

// A dialog-like widget that isn't a full standalone dialog but is used to build
// a subset of a dialogue.
class DialogFragment : public QWidget
{
    Q_OBJECT

public:
    explicit DialogFragment(QWidget *parent = 0);

    virtual ~DialogFragment(void);

    bool is_enabled(void) const;

    bool has_unsaved_changes(void) const;

    void set_enabled(const bool isEnabled);

    void set_name(const QString &newName);

    const QString& name(void) const;

    void set_data_filename(const QString &filename);

    void set_unsaved_changes_flag(const bool areUnsavedChanges);

    void lock_unsaved_changes_flag(const bool isLocked);

    const QString& data_filename(void) const;

signals:
    // Emitted when the dialog's enabled state is set. Note that this is emitted regardless
    // of the state's previous value.
    void enabled_state_set(const bool isEnabled);

    // Emitted when the dialog's unsaved changes flag is modified. Note that this is emitted
    // regardless of the flag's previous value.
    void unsaved_changes_flag_changed(const bool areUnsavedChanges);

    // Emitted when the dialog's data file name is modified. Note that this is emitted
    // regardless of the name's previous value.
    void data_filename_changed(const QString &newName);

    // Emitted when any of the dialog's data changes. However, it's up to the dialog to
    // emit this signal; the base class doesn't.
    void data_changed(void);

protected:
    void update_window_title(void);

    QMenuBar *menuBar = nullptr;

private:
    void keyPressEvent(QKeyEvent *event);

    // Whether the dialog's functionality is enabled in VCS.
    bool _isEnabled = false;

    // Indicates whether modifications to the dialog's data have been saved.
    bool _areUnsavedChanges = false;

    // If true, the data_changed() signal won't modify the unsaved changes flag.
    bool _isUnsavedChangesFlagLocked = false;

    // The dialog's name, to be shown in its window title.
    QString _name = "";

    // The name of the file from which this dialog's parameters were fetched
    // from or saved to.
    QString _dataFilename = "";
};

#endif
