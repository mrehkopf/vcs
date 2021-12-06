/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QFileInfo>
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"
#include "common/globals.h"

VCSBaseDialog::VCSBaseDialog(QWidget *parent) : QDialog(parent)
{
    connect(this, &VCSBaseDialog::unsaved_changes_flag_changed, this, [this]
    {
        this->update_window_title();
    });

    connect(this, &VCSBaseDialog::data_changed, this, [this]
    {
        this->set_unsaved_changes_flag(true);
    });

    connect(this, &VCSBaseDialog::data_filename_changed, this, [this]
    {
        this->set_unsaved_changes_flag(false);
    });

    return;
}

VCSBaseDialog::~VCSBaseDialog(void)
{
    return;
}

void VCSBaseDialog::open(void)
{
    this->show();
    this->activateWindow();
    this->raise();

    return;
}

const QString& VCSBaseDialog::name(void) const
{
    return this->_name;
}

bool VCSBaseDialog::has_unsaved_changes(void) const
{
    return this->_areUnsavedChanges;
}

bool VCSBaseDialog::is_enabled(void) const
{
    return this->_isEnabled;
}

void VCSBaseDialog::set_enabled(const bool isEnabled)
{
    this->_isEnabled = isEnabled;
    emit this->enabled_state_set(isEnabled);

    return;
}

void VCSBaseDialog::set_name(const QString &newName)
{
    this->_name = newName;
    this->update_window_title();

    return;
}

void VCSBaseDialog::set_data_filename(const QString &filename)
{
    this->_dataFilename = filename;
    this->update_window_title();
    emit this->data_filename_changed(filename);

    return;
}

void VCSBaseDialog::set_unsaved_changes_flag(const bool areUnsavedChanges)
{
    if (this->_isUnsavedChangesFlagLocked)
    {
        return;
    }

    this->_areUnsavedChanges = areUnsavedChanges;
    emit this->unsaved_changes_flag_changed(areUnsavedChanges);

    return;
}

void VCSBaseDialog::lock_unsaved_changes_flag(const bool isBlocked)
{
    this->_isUnsavedChangesFlagLocked = isBlocked;

    return;
}

const QString& VCSBaseDialog::data_filename(void) const
{
    return this->_dataFilename;
}

void VCSBaseDialog::update_window_title(void)
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
