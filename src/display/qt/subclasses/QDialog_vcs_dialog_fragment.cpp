/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QFileInfo>
#include <QKeyEvent>
#include "display/qt/subclasses/QDialog_vcs_dialog_fragment.h"
#include "common/globals.h"

VCSDialogFragment::VCSDialogFragment(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::Window);

    connect(this, &VCSDialogFragment::unsaved_changes_flag_changed, this, [this]
    {
        this->update_window_title();
    });

    connect(this, &VCSDialogFragment::data_changed, this, [this]
    {
        this->set_unsaved_changes_flag(true);
    });

    connect(this, &VCSDialogFragment::data_filename_changed, this, [this]
    {
        this->set_unsaved_changes_flag(false);
    });

    return;
}

VCSDialogFragment::~VCSDialogFragment(void)
{
    return;
}

const QString& VCSDialogFragment::name(void) const
{
    return this->_name;
}

bool VCSDialogFragment::has_unsaved_changes(void) const
{
    return this->_areUnsavedChanges;
}

bool VCSDialogFragment::is_enabled(void) const
{
    return this->_isEnabled;
}

void VCSDialogFragment::set_enabled(const bool isEnabled)
{
    this->_isEnabled = isEnabled;
    emit this->enabled_state_set(isEnabled);

    return;
}

void VCSDialogFragment::set_name(const QString &newName)
{
    this->_name = newName;
    this->update_window_title();

    return;
}

void VCSDialogFragment::set_data_filename(const QString &filename)
{
    this->_dataFilename = filename;
    this->update_window_title();
    emit this->data_filename_changed(filename);

    return;
}

void VCSDialogFragment::set_unsaved_changes_flag(const bool areUnsavedChanges)
{
    if (this->_isUnsavedChangesFlagLocked)
    {
        return;
    }

    this->_areUnsavedChanges = areUnsavedChanges;
    emit this->unsaved_changes_flag_changed(areUnsavedChanges);

    return;
}

void VCSDialogFragment::lock_unsaved_changes_flag(const bool isLocked)
{
    this->_isUnsavedChangesFlagLocked = isLocked;

    return;
}

const QString& VCSDialogFragment::data_filename(void) const
{
    return this->_dataFilename;
}

void VCSDialogFragment::update_window_title(void)
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

void VCSDialogFragment::keyPressEvent(QKeyEvent *event)
{
    // Consume (ignore) the Esc key to prevent it from closing the dialog. This
    // is done since the dialog may be embedded as part of another dialog and we
    // don't want it closing in that context. It's a kludge solution for now.
    if (event->key() == Qt::Key_Escape)
    {
        event->accept();
    }
}
