#include <QWindow>
#include "display/display.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/persistent_settings.h"
#include "window_renderer.h"
#include "ui_window_renderer.h"

WindowRenderer::WindowRenderer(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::WindowRenderer)
{
    ui->setupUi(this);

    connect(ui->comboBox_renderer, &QComboBox::currentTextChanged, this, [parent](const QString &rendererName)
    {
        NBENE(("WindowRenderer: Unimplemented functionality."));
        // kd_set_renderer(rendererName.toStdString());)

        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "Renderer", rendererName);
    });

    // Restore the previous persistent setting.
    ui->comboBox_renderer->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "Renderer", ui->comboBox_renderer->itemText(0)).toString());
}

WindowRenderer::~WindowRenderer()
{
    delete ui;
}
