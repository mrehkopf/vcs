/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QFileInfo>
#include <QKeyEvent>
#include "display/qt/widgets/QDialog_vcs_dialog_fragment.h"
#include "common/globals.h"

DialogFragment::DialogFragment(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::Window);

    connect(this, &DialogFragment::unsaved_changes_flag_changed, this, [this]
    {
        this->update_window_title();
    });

    connect(this, &DialogFragment::data_changed, this, [this]
    {
        this->set_unsaved_changes_flag(true);
    });

    connect(this, &DialogFragment::data_filename_changed, this, [this]
    {
        this->set_unsaved_changes_flag(false);
    });

    return;
}

DialogFragment::~DialogFragment(void)
{
    return;
}

const QString& DialogFragment::name(void) const
{
    return this->_name;
}

bool DialogFragment::has_unsaved_changes(void) const
{
    return this->_areUnsavedChanges;
}

bool DialogFragment::is_enabled(void) const
{
    return this->_isEnabled;
}

void DialogFragment::set_enabled(const bool isEnabled)
{
    this->_isEnabled = isEnabled;
    emit this->enabled_state_set(isEnabled);

    return;
}

void DialogFragment::set_name(const QString &newName)
{
    this->_name = newName;
    this->update_window_title();

    return;
}

void DialogFragment::set_data_filename(const QString &filename)
{
    this->_dataFilename = filename;
    this->update_window_title();
    emit this->data_filename_changed(filename);

    return;
}

void DialogFragment::set_unsaved_changes_flag(const bool areUnsavedChanges)
{
    if (this->_isUnsavedChangesFlagLocked)
    {
        return;
    }

    this->_areUnsavedChanges = areUnsavedChanges;
    emit this->unsaved_changes_flag_changed(areUnsavedChanges);

    return;
}

void DialogFragment::lock_unsaved_changes_flag(const bool isLocked)
{
    this->_isUnsavedChangesFlagLocked = isLocked;

    return;
}

const QString& DialogFragment::data_filename(void) const
{
    return this->_dataFilename;
}

void DialogFragment::update_window_title(void)
{
    QString title = "";
    const QString unsavedChangesMarker = (this->_areUnsavedChanges? "*" : "");

    if (this->_dataFilename.isEmpty())
    {
        title = QString("%1 - %2").arg(this->_name).arg(PROGRAM_NAME);
    }
    else
    {
        const QString baseFilename = QFileInfo(this->_dataFilename).baseName();
        title = QString("%1 - %2 - %3").arg(baseFilename).arg(this->_name).arg(PROGRAM_NAME);
    }

    this->setWindowTitle(unsavedChangesMarker + title);

    return;
}

void DialogFragment::keyPressEvent(QKeyEvent *event)
{
    // Consume (ignore) the Esc key to prevent it from closing the dialog. This
    // is done since the dialog may be embedded as part of another dialog and we
    // don't want it closing in that context. It's a kludge solution for now.
    if (event->key() == Qt::Key_Escape)
    {
        event->accept();
    }
}
