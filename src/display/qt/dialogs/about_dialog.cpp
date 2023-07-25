/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "ui_about_dialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    this->set_name("About VCS");

    // Initialize the table of information. Note that this also sets
    // the vertical order in which the table's parameters are shown.
    {
        ui->tableWidget->modify_property("Version", PROGRAM_VERSION_STRING);
        ui->tableWidget->modify_property("Is release build",
            #ifdef VCS_RELEASE_BUILD
                "Yes"
            #else
                "No"
            #endif
        );
        ui->tableWidget->modify_property("Build date", __DATE__);
    }

    return;
}

AboutDialog::~AboutDialog()
{
    delete ui;

    return;
}
