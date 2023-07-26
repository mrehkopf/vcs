#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include "display/qt/windows/control_panel/capture.h"
#include "display/qt/windows/control_panel/output.h"
#include "display/qt/windows/control_panel/video_presets.h"
#include "display/qt/windows/control_panel/filter_graph.h"
#include "display/qt/windows/control_panel/about_vcs.h"
#include "display/qt/windows/control_panel/overlay.h"
#include "display/qt/windows/output_window.h"
#include "control_panel.h"
#include "ui_control_panel.h"

ControlPanelWindow::ControlPanelWindow(OutputWindow *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanelWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Control panel - VCS");
    this->setWindowFlags(Qt::Window);

    this->captureDialog = new CaptureDialog(this);
    this->windowOptionsDialog = new WindowOptionsDialog(this);
    this->filterGraphDialog = new FilterGraphDialog(this);
    this->videoPresetsDialog = new VideoPresetsDialog(this);
    this->overlayDialog = new OverlayDialog(this);
    this->aboutDialog = new AboutDialog(this);

    // Populate the side navi.
    {
        QVBoxLayout *const layout = dynamic_cast<QVBoxLayout*>(ui->buttonsContainer->layout());

        const auto add_navi_button = [this, layout](const QString &label, VCSDialogFragment *const dialog)->QPushButton*
        {
            auto *const naviButton = new QPushButton(label);
            naviButton->setFlat(true);
            naviButton->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(naviButton);

            connect(naviButton, &QPushButton::pressed, this, [this, naviButton, dialog]
            {
                ui->contentsScroller->takeWidget();
                ui->contentsScroller->setWidget(dialog);
                dialog->show();

                foreach (auto *const childButton, this->ui->buttonsContainer->children())
                {
                    childButton->setProperty("isSelected", ((childButton == naviButton)? "true" : "false"));
                    this->style()->polish(dynamic_cast<QWidget*>(childButton));
                };
            });

            return naviButton;
        };

        auto *const defaultButton = add_navi_button("Capture", this->captureDialog);
        add_navi_button("Output", this->windowOptionsDialog);
        add_navi_button("Filter graph", this->filterGraphDialog);
        add_navi_button("Video presets", this->videoPresetsDialog);
        add_navi_button("Overlay", this->overlayDialog);
        add_navi_button("About VCS", this->aboutDialog);

        // Push the buttons against the top edge of the container.
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        defaultButton->click();
    }
}

ControlPanelWindow::~ControlPanelWindow()
{
    delete ui;
}
