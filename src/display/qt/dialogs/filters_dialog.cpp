#include <QMenuBar>
#include "filters_dialog.h"
#include "ui_filters_dialog.h"

FiltersDialog::FiltersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FiltersDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Filters");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // File...
        {
            QMenu *fileMenu = new QMenu("File", this);

            menubar->addMenu(fileMenu);
        }

        // Help...
        {
            QMenu *fileMenu = new QMenu("Help", this);

            fileMenu->addAction("About...");
            fileMenu->actions().at(0)->setEnabled(false); /// TODO: Add the actual help.

            menubar->addMenu(fileMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    return;
}

FiltersDialog::~FiltersDialog()
{
    delete ui;

    return;
}
