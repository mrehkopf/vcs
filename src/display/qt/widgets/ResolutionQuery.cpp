/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * A simple dialog that queries the user for a resolution and returns it.
 *
 */

#include "display/qt/widgets/ResolutionQuery.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "ui_ResolutionQuery.h"

ResolutionQuery::ResolutionQuery(const QString title, resolution_s *const r, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResolutionQuery),
    resolution(r)
{
    k_assert(
        (resolution != nullptr),
        "Expected a valid resolution pointer as an argument to the resolution dialog, but got null."
    );

    ui->setupUi(this);
    this->setWindowTitle(title);

    // Set the GUI controls to their proper initial values.
    {
        ui->spinBox_x->setMinimum(MIN_OUTPUT_WIDTH);
        ui->spinBox_x->setMaximum(MAX_OUTPUT_WIDTH);
        ui->spinBox_y->setMinimum(MIN_OUTPUT_HEIGHT);
        ui->spinBox_y->setMaximum(MAX_OUTPUT_HEIGHT);
        ui->spinBox_x->setValue(resolution->w);
        ui->spinBox_y->setValue(resolution->h);
    }

    // Connect GUI controls to consequences for operating them.
    {
        // Update the target resolution, and exit the dialog.
        connect(ui->pushButton_ok, &QPushButton::clicked, this, [this]
        {
            resolution->w = ui->spinBox_x->value();
            resolution->h = ui->spinBox_y->value();

            this->accept();
        });

        // Exit the dialog without modifying the target resolution.
        connect(ui->pushButton_cancel, &QPushButton::clicked, this, [this]
        {
            this->reject();
        });
    }

    return;
}

ResolutionQuery::~ResolutionQuery(void)
{
    delete ui;
    ui = nullptr;

    return;
}
