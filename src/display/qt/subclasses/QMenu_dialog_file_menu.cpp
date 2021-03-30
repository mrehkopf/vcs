/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QMessageBox>
#include "display/qt/subclasses/QMenu_dialog_file_menu.h"
#include "common/globals.h"

DialogFileMenu::DialogFileMenu(VCSBaseDialog *const parentDialog) :
    QMenu(parentDialog),
    parentDialog(parentDialog)
{
    k_assert(parentDialog, "A base dialog pointer is required.");

    this->setTitle("File");

    QAction *save = new QAction("Save", this);
    QAction *saveAs = new QAction("Save as...", this);
    QAction *open = new QAction("Open...", this);
    QAction *close = new QAction("Close", this);

    save->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    open->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));

    // Initial state when no file has been opened.
    save->setEnabled(false);
    saveAs->setEnabled(true);
    open->setEnabled(true);
    close->setEnabled(false);

    this->addAction(save);
    this->addAction(saveAs);
    this->addSeparator();
    this->addAction(open);
    this->addSeparator();
    this->addAction(close);

    connect(saveAs, &QAction::triggered, this, [=]
    {
        emit this->save_as(save->data().toString());
    });

    connect(save, &QAction::triggered, this, [=]
    {
        emit this->save(save->data().toString());
    });

    connect(open, &QAction::triggered, this, [=]
    {
        emit this->open();
    });

    connect(close, &QAction::triggered, this, [=]
    {
        if (parentDialog->has_unsaved_changes() &&
            (QMessageBox::question(parentDialog,
                                   "Close file?",
                                   "This file has unsaved changes. Close it anyway?",
                                   (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::No))
        {
            return;
        }

        emit this->close(close->data().toString());
    });

    connect(this->parentDialog, &VCSBaseDialog::data_filename_changed, this, [=](const QString &filename)
    {
        const QString shortFilename = QFileInfo(filename).fileName();

        save->setEnabled(!filename.isEmpty());
        save->setData(filename);

        close->setEnabled(!filename.isEmpty());
        close->setData(filename);
        close->setText(filename.isEmpty()
                       ? "Close"
                       : QString("Close \"%1\"").arg(shortFilename));
    });

    return;
}

DialogFileMenu::~DialogFileMenu(void)
{
    return;
}

