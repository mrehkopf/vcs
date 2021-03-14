/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QDIALOG_VCS_BASE_DIALOG_H
#define VCS_DISPLAY_QT_SUBCLASSES_QDIALOG_VCS_BASE_DIALOG_H

#include <QDialog>

class QMenuBar;

class VCSBaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VCSBaseDialog(QWidget *parent = 0);

    virtual ~VCSBaseDialog(void);

    bool is_enabled(void) const;

    void set_enabled(const bool isEnabled);

    void set_name(const QString &newName);

    void set_data_filename(const QString &filename);

    const QString& data_filename(void) const;

signals:
    // Emitted when the dialog's enabled state is set via this->set_enabled().
    void enabled_state_set(const bool isEnabled);

    // Emitted when set_data_filename() is called.
    void data_filename_changed(const QString &newName);

protected:
    void update_window_title(void);

    void save_data_file(const QString &filename);

    void load_data_file(const QString &filename);

    QMenuBar *menuBar = nullptr;

private:
    // Whether the dialog's functionality is enabled in VCS.
    bool _isEnabled = false;

    // The dialog's name, to be shown in its window title.
    QString _name = "";

    // The name of the file from which this dialog's parameters were fetched
    // from or saved to.
    QString _dataFilename = "";
};

#endif
