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
    connect(this, &VCSBaseDialog::unsaved_changes_flag_changed, this, [this](const bool areUnsavedChanges)
    {
        this->update_window_title();
    });

    return;
}

VCSBaseDialog::~VCSBaseDialog(void)
{
    return;
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

void VCSBaseDialog::set_unsaved_changes(const bool areUnsavedChanges)
{
    this->_areUnsavedChanges = areUnsavedChanges;
    emit this->unsaved_changes_flag_changed(areUnsavedChanges);
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
