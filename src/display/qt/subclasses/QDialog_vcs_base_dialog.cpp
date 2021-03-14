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

const QString& VCSBaseDialog::data_filename(void) const
{
    return this->_dataFilename;
}

void VCSBaseDialog::update_window_title(void)
{
    if (this->_dataFilename.isEmpty())
    {
        this->setWindowTitle(QString("%1 - %2").arg(this->_name).arg(PROGRAM_NAME));
    }
    else
    {
        const QString baseFilename = QFileInfo(this->_dataFilename).baseName();
        this->setWindowTitle(QString("%1 - %2 - %3").arg(baseFilename).arg(this->_name).arg(PROGRAM_NAME));
    }

    return;
}
