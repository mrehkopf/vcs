/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS alias dialog
 *
 */

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QLabel>
#include "../../persistent_settings/persistent_settings.h"
#include "ui_d_alias_dialog.h"
#include "d_alias_dialog.h"
#include "../../capture/capture.h"
#include "../display.h"

/*
 * TODOS:
 *
 * - at the moment, this is just a barebones dialog to give the user some basic info
 *   of the aliases in use. could be better.
 *
 */

AliasDialog::AliasDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AliasDialog)
{
    ui->setupUi(this);

    ui->scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop);

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    resize(kpers_value_of("aliases", INI_GROUP_GEOMETRY, size()).toSize());

    return;
}

AliasDialog::~AliasDialog()
{
    kpers_set_value("aliases", INI_GROUP_GEOMETRY, size());

    delete ui; ui = nullptr;

    return;
}

// Add the given alias into the GUI.
//
void AliasDialog::receive_new_alias(const mode_alias_s a)
{
    QLabel *l = new QLabel;

    l->setObjectName(QString("label_#%1").arg(ui->scrollAreaWidgetContents->layout()->count()));
    l->setText(QString("%3 x %4 = %1 x %2").arg(a.from.w)
                                           .arg(a.from.h)
                                           .arg(a.to.w)
                                           .arg(a.to.h));

    ui->scrollAreaWidgetContents->layout()->addWidget(l);   // Will delete the allocation on exit.

    return;
}

// Clears the GUI's list of known aliases. Will be called by the alias handler
// (currently, capture.cpp) when the user has asked to reset aliases.
//
void AliasDialog::clear_known_aliases()
{
    QList<QWidget*> deletableLabels;

    /// Temp hack. Find all alias labels, then clear them.
    for (int i = 0; i < ui->scrollAreaWidgetContents->layout()->count(); i++)
    {
        QWidget *const w = ui->scrollAreaWidgetContents->layout()->itemAt(i)->widget();
        if (w->objectName().startsWith("label_#"))
        {
            deletableLabels.push_back(w);
        }
    }
    for (auto *label: deletableLabels)
    {
        delete label;
    }

    return;
}

void AliasDialog::on_pushButton_load_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Select the file to load aliases from", "",
                                                    "Alias files (*.vcsa);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    kc_load_aliases(filename, false);

    return;
}
